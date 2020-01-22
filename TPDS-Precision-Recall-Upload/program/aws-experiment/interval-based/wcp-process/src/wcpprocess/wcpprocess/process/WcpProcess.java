package wcpprocess.process; /**
 * Created by duongnn on 11/4/18.
 */


import predicatedetectionlib.common.*;
import predicatedetectionlib.common.predicate.CandidateMessageContent;
import predicatedetectionlib.versioning.MonitorVectorClock;
import wcpprocess.helpers.Utilities;
import wcpprocess.protobuf.WcpMessageProtos.*;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.util.Vector;
import java.util.concurrent.ConcurrentLinkedQueue;

import static predicatedetectionlib.common.CommonUtils.log;
import static wcpprocess.helpers.Utilities.getRandomNumberInRange;
import static wcpprocess.process.BasicProcess.*;

/**
 * Wcp application
     while(1){
         do local computation or sleep
         send message with probability A
         processing incoming message
         change x_i with probability B
         if local predicate is true, send candidate to monitor (with interval, so we
         have to wait until interval is ended)
     }

   In this application, the interval is not split by communication event
 */

public class WcpProcess implements Runnable{

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

    WcpProcess(){
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
        // check if you have to sendCandidate
//        boolean needToSendCandidate = isLocalStateTrue();
        boolean sendCandNow = needToSendCandidate();

        if(sendCandNow){
            // form candidate and send to monitor
            HvcInterval outgoingHvcInterval = new HvcInterval(getPhysicalStartTime(), getStartPoint(), getProcHvc());
            LocalSnapshotContent lsc = new LocalSnapshotContent(getLocalState());
            String predicateName = "0"; // assume we have predicate with ID 0
            incrementCandidateSeqNumber();

            LocalSnapshot outgoingLocalSnapshot =
                    new LocalSnapshot(outgoingHvcInterval, getCandidateSeqNumber(), lsc);
            CandidateMessageContent outgoingCandidateMessageContent =
                    new CandidateMessageContent(predicateName, outgoingLocalSnapshot);

            try {
                int monitorId = 0;
                sendCandidateToMonitor(outgoingCandidateMessageContent, monitorId);

                HvcElement endPoint = outgoingHvcInterval.getEndPoint().getPrimaryElement();
                HvcElement startPoint = outgoingHvcInterval.getStartPoint().getPrimaryElement();

                long intervalLengthMs = endPoint.getTimeValueInMs() - startPoint.getTimeValueInMs();
                //long intervalLengthNs = endPoint.getTimeValue() - startPoint.getTimeValue();

                totalCandidateCount ++;
                totalCandidateIntervalLength += intervalLengthMs;

                // mark pendingIntervalStatus
                setPendingIntervalFlag(false);

            }catch(Exception e){
                log("changeLocalState: ERROR " + e.getMessage());
                e.printStackTrace();
            }
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
                if (getPendingIntervalFlag() == false) {
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

    // check if you need to send a candidate
    // return
    //      true when
    //          there is a pending interval (thus a pending candidate), and
    //          the interval has expired (i.e. currentTime - intervalStart > intervalLength)
    //      false otherwise, i.e.
    //          no pending interval, or
    //          there is pending interval but not yet expires
    // note: pending interval implies local state = true
    private static boolean needToSendCandidate(){
        if(getPendingIntervalStatus() == PENDING_INTERVAL_STATUS_EXPIRED)
            return true;

        return false;

//        if(getPendingIntervalFlag() == false) {
//            return false;
//        }
////        if(! isLocalStateTrue())
////            return false;
//
//        // there is a pending interval, we check if interval length has been reached or not
//        // note the unit of time for interval length is microsecond
//        long SCALE = 1000;  // one microsecond is 1000 nanosecond
//        if((System.nanoTime() - getPendingIntervalStartNs()) >= getBaseIntervalLength()*SCALE){
//            // pending interval expires
//            return true;
//        }else{
//            // pending interval has not yet expired
//            return false;
//        }
    }


    // send a candidate to monitor
    // since this is not relevant to application logic, we will not update HVC and other variables in this function
    private synchronized static void sendCandidateToMonitor(
            CandidateMessageContent candidateMessageContent,
            int monitorId)
            throws IOException, InterruptedException {

        // retrieve the existing connection to monitor
        String monitorIdStr = String.valueOf(monitorId);
        SocketChannel monitorClient = monitorIdConnectionMap.get(monitorIdStr);

        // it is possible the connection is closed by monitor, then we need to reopen it
        if(monitorClient.socket().isClosed()){

            log(" sendCandidateToMonitor: connection is already closed. Reopening it");

            // close
            monitorClient.close();

            // and reopen it
            String[] monitorAddr = monitorIdAddrMap.get(monitorIdStr).split(":");
            InetSocketAddress monitorAddrSocket = new InetSocketAddress(InetAddress.getByName(monitorAddr[0]),
                    Integer.valueOf(monitorAddr[1]));
//                log("    Reconnecting to Monitor on at " + monitorAddrSocket +
//                        " for sequence number " + localSnapshot.getSequenceNumber() + " ...");

            monitorClient = SocketChannel.open(monitorAddrSocket);
            monitorIdConnectionMap.put(monitorIdStr, monitorClient);

        }else{
//                log(" sendCandidateToMonitor: connection is still active");
        }

        int[] cmcLength = new int[1];
        byte[] message = candidateMessageContent.toBytes(cmcLength);

//        log(" Process" + getProcessId() + ": sending "
//                + cmcLength[0] + " bytes to monitor " + monitorIdStr
//                + " for predicate " + candidateMessageContent.getPredicateName());
//        log(candidateMessageContent.toString(0));

        ByteBuffer buffer = ByteBuffer.wrap(message);

        int numberOfBytesWritten;

        // we should have one server thread use one connection at a time
        synchronized (monitorClient) {
            numberOfBytesWritten = monitorClient.write(buffer);
        }

//            log("      " + numberOfBytesWritten + " bytes are sent");

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

    public static void incrementProcHvc(){
        getProcHvc().incrementMyLocalClock();
    }

}
