package wcpprocess.process; /**
 * Created by duongnn on 11/4/18.
 */


import predicatedetectionlib.common.*;
import predicatedetectionlib.versioning.MonitorVectorClock;
import wcpprocess.helpers.Utilities;
import wcpprocess.protobuf.WcpMessageProtos.*;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Vector;
import java.util.concurrent.ConcurrentLinkedQueue;

import static predicatedetectionlib.common.CommonUtils.log;
import static wcpprocess.helpers.Utilities.getRandomNumberInRange;
import static wcpprocess.process.BasicProcess.*;

/**
 * Similar to WcpProcess, but we split the interval when there is communication
 */

public class WcpProcessSplitInterval implements Runnable{

    // help computing actual beta and actual interval length
    static long totalCandidateIntervalLength;
    static int totalCandidateCount;
    static int xFlippedToTrueCount;
    static int xFlippedToFalseCount;

//    // help computing alpha
//    static int messageSentCount;
//    static int messageReceivedCount;

    static int iterationCount;

    static long sleepDurationMicroSec;
    static long sleepDurationNanoSec;
    static long sleepDurationMilliSec;

    WcpProcessSplitInterval(){
        totalCandidateCount = 0;
        totalCandidateIntervalLength = 0;
        xFlippedToFalseCount = 0;
        xFlippedToTrueCount = 0;
        iterationCount = 0;

        sleepDurationMicroSec = getLocalStepComputationTimeMuS();
        sleepDurationNanoSec = (sleepDurationMicroSec%1000)*1000;
        sleepDurationMilliSec = sleepDurationMicroSec/1000;

        System.out.println("sleepDurationMicroSec = " + sleepDurationMicroSec);
        System.out.println("sleepDurationMilliSec = " + sleepDurationMilliSec);
        System.out.println("sleepDurationNanoSec  = " + sleepDurationNanoSec);

    }

    public void run(){

        while(! BasicProcess.stopThreadNow){
            // increase iterationCount at this point
            // to avoid divide by 0 if thread is interrupted before the first iteration completes
            iterationCount ++;

            // process received messages
            processReceivedMessages(getReceivedMessagesQueue());

            // send message
            sendMessageToRandomProcess();

            // perform local computation
            doLocalComputation();


        }
    }


    /**
     * send a message to a random destination (other process)
     * the frequency of sending message is alpha
     */
    static void sendMessageToRandomProcess(){
        if(getRandomNumberInRange(1,100) * 1.0 <= getAlpha() * 100.0){

            // split the pending interval if there is any
            splitPendingIntervalIfNeeded();

            // send message to a random other process
            int destination;

            // pick a destination that is not yourself
            while((destination = getRandomNumberInRange(0, getNumberOfProcesses() - 1)) == getProcessId());

            // prepare message
            MessageHVC.Builder msgHvcBuilder = MessageHVC.newBuilder().setSenderProcId(getProcessId());
            HVC hvc = getProcHvc();
            for(int i = 0; i < hvc.getSize(); i ++){
                msgHvcBuilder.addHvcTimestamp(hvc.getHvcElement(i).getTimeValue());
            }

            String messageContentStr = "sent=" + messageSentCount
                    + " received=" + messageReceivedCount
                    + " total=" + (messageSentCount + messageReceivedCount);

            // prepare message
            ProcessMessage msg = ProcessMessage.newBuilder()
                    .setSenderHvc(msgHvcBuilder.build())
                    .setMessageContent(messageContentStr)
                    .build();

            // Now we can increase your HVC with assumption that the message has been sent out.
            // Even if the message has not been sent out, since the message content is fixed now,
            // we can think as if the message delivery is delayed.
            incrementProcHvc();

            // send message
            try {
                sendMessageToDestinationThroughSeparateThread(msg, destination);

//                log("send: " + getProcessId() + " -> " + destination + ": " + messageContentStr);

//                messageSentCount ++;

            }catch(Exception e){
                log("sendMessageToRandomProcess: ERROR: " +
                        "node " + getProcessId() + " could not send message to node " + destination);
                log(e.getMessage());
            }
        }else{
            // we may want to increase HVC
            incrementProcHvc();

        }
    }

    // process message(s) that you have received
    static void processReceivedMessages(ConcurrentLinkedQueue<ProcessMessage> messageQueue){
        if(messageQueue.isEmpty()){
            // no message to be processed
            // we may want to increase HVC before returning
            incrementProcHvc();

            return;
        }

        while(! messageQueue.isEmpty()){
            ProcessMessage msg = messageQueue.poll();

            messageReceivedCount ++;

            // split the pending interval if there is any
            splitPendingIntervalIfNeeded();

            // construct HVC from message
            MessageHVC hvcContainedInMsg = msg.getSenderHvc();
            int msgHvcSize = hvcContainedInMsg.getHvcTimestampCount();
            Vector<HvcElement> msgHvcElements = new Vector<>();
            for(int i = 0; i < msgHvcSize; i ++){
                long timestamp = hvcContainedInMsg.getHvcTimestamp(i);
                HvcElement hvcElement = new HvcElement();
                hvcElement.setTimeValue(timestamp);
                msgHvcElements.addElement(hvcElement);
            }
            int senderNodeId = hvcContainedInMsg.getSenderProcId();
            HVC hvc = new HVC(senderNodeId, getEpsilon(), msgHvcElements);

            // update HVC
            Vector<HvcElement> maxHvcElement = HVC.getMaxHvcTimestamp(getProcHvc(), hvc);
            getProcHvc().setHvcTimestamp(maxHvcElement);
            getProcHvc().updateActiveSize();
            incrementProcHvc();

//            // read message content
//            log("received: " + senderNodeId + " -> " + getProcessId() + ": " + msg.getMessageContent());
//            log(getProcHvc().toStringHvcFull(2));

        }
    }

    static void doLocalComputation(){
        // change local state
        // and generate candidate if appropriate
        changeLocalState();

        // sleep to emulate other computation
        try {

            Thread.sleep(sleepDurationMilliSec, (int) sleepDurationNanoSec);

        }catch (InterruptedException ie){
            System.out.println(" doLocalComputation: Get InterruptedException when sleeping");
        }
    }

    static void changeLocalState(){
        // check if there is any pending interval/candidate that has expired.
        // If so, send it out
        int pendingIntervalStatus = getPendingIntervalStatus();

        if(pendingIntervalStatus == PENDING_INTERVAL_STATUS_EXPIRED){
            sendCandidateAndCount();
        }

        // change local state:
        //   change x_i where i is procId into 1 with probability specified
        //   and update vector clock
        // local state should not be changed if there is a pending interval

        if(getPendingIntervalFlag() == false) {
            MonitorVectorClock dumpMvc = new MonitorVectorClock();
            if (Utilities.getRandomNumberInRange(1, 100) * 1.0 <= getBeta() * 100) {
                getLocalState().put("x_" + getProcessId(), dumpMvc, "1");

                // update HVC
                incrementProcHvc();

                // if there is no pending interval, we have to mark the start point and status
                if (getPendingIntervalFlag() == false) { // this should always be met
                    advanceStartPointToCurrentHvc();
                    setPendingIntervalFlag(true);

                }

                xFlippedToTrueCount++;

            } else {
                getLocalState().put("x_" + getProcessId(), dumpMvc, "0");

                // update HVC
                incrementProcHvc();

                xFlippedToFalseCount++;
            }
        }else{
            // we may want to increase HVC
            incrementProcHvc();
        }

    }

    public static void writeResultsToFile(String summaryStr, String filename){
        try {
            long duration_time_ms = stopTimeMs - startTimeMs;

            double totalIntvLen_totalDuration_ratio = (1.0*totalCandidateIntervalLength)/(1.0*duration_time_ms);
            double average_intv_len_ms = (1.0*totalCandidateIntervalLength)/(1.0*totalCandidateCount);
            double average_iteration_length_ms = (1.0*duration_time_ms)/(1.0*iterationCount);
            double average_intv_len_in_iteration = average_intv_len_ms/average_iteration_length_ms;
            double practical_beta = (1.0*xFlippedToTrueCount)/(1.0*(xFlippedToTrueCount + xFlippedToFalseCount));
            double practical_alpha = (1.0*messageSentCount)/(1.0*iterationCount);

            PrintWriter printWriter = new PrintWriter(new FileWriter(filename), true);
            printWriter.printf("%s", summaryStr);

            // fist line is total number of snapshot found
            printWriter.println("# processId = " + getProcessId());
            printWriter.println("#  totalCandidateCount = " + totalCandidateCount);
            printWriter.println("#  totalCandidateIntervalLength = " + totalCandidateIntervalLength);
            printWriter.println("#  startTimeMs = " + startTimeMs);
            printWriter.println("#  stopTimeMs = " + stopTimeMs);
            printWriter.println("#  duration_time_ms = " + duration_time_ms);
            printWriter.println("#  xFlippedToTrueCount = " + xFlippedToTrueCount);
            printWriter.println("#  xFlippedToFalseCount = " + xFlippedToFalseCount);
            printWriter.println("#  messageSentCount = " + messageSentCount);
            printWriter.println("#  messageReceivedCount = " + messageReceivedCount);
            printWriter.println("#  iterationCount = " + iterationCount);
            printWriter.println("#    average_intv_len_ms = " + average_intv_len_ms);
            printWriter.println("#    average_iteration_length_ms = " + average_iteration_length_ms);
            printWriter.println("#    average_intv_len_in_iteration = " + average_intv_len_in_iteration);
            printWriter.println("#    totalIntvLen_totalDuration_ratio = " + totalIntvLen_totalDuration_ratio);
            printWriter.println("#    practical_alpha = " + practical_alpha);
            printWriter.println("#    practical_beta = " + practical_beta);
            printWriter.println("#");
            printWriter.println("#");

            printWriter.close();

        }catch(IOException e){
            System.out.println(" Could not open file " + filename + " to write process data");
            System.out.println(e.getMessage());
        }

    }

    // increment process HVC
    public static void incrementProcHvc(){
        getProcHvc().incrementMyLocalClock();
    }

    // if there is any pending interval, split it
    private static void splitPendingIntervalIfNeeded(){
        int pendingIntervalStatus = getPendingIntervalStatus();
        switch(pendingIntervalStatus){
            case PENDING_INTERVAL_STATUS_EMPTY:
                // no pending interval, do nothing
                return;

            case PENDING_INTERVAL_STATUS_EXPIRED:
                // the pending interval has expired: send the interval out
                sendCandidateAndCount();

                return;
            case PENDING_INTERVAL_STATUS_ACTIVE:
                // the pending interval has not yet expired: split it, send out the first half, leave the second half
                sendCandidateAndCount();

                // mark pendingIntervalFlag as true since there is still another half
                // also update the start point
                advanceStartPointToCurrentHvc();
                setPendingIntervalFlag(true);

                return;
        }
    }

    private static void sendCandidateAndCount(){
        long intervalLength = sendCandidateNow();
        if(intervalLength >= 0){
            totalCandidateIntervalLength += intervalLength;
            totalCandidateCount ++;
        }
    }

}
