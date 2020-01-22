package wcpprocess.process; /**
 * Created by duongnn on 11/2/18.
 */

import com.google.common.collect.HashBasedTable;
import com.google.common.collect.Table;
import joptsimple.OptionParser;
import joptsimple.OptionSet;
import joptsimple.OptionSpec;
import predicatedetectionlib.common.*;
import predicatedetectionlib.common.predicate.CandidateMessageContent;
import predicatedetectionlib.versioning.MonitorVectorClock;
import wcpprocess.protobuf.WcpMessageProtos.*;

import java.io.*;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.util.HashMap;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.LinkedBlockingQueue;

import static predicatedetectionlib.common.CommonUtils.log;

/**
 * This class provide the basic model and functionality for a process
 */

public class BasicProcess {
    volatile static public boolean stopThreadNow;    // for informing threads to stop

    static long startTimeMs;
    static long stopTimeMs;

    static private int processId;

    volatile static private HVC procHvc; // HVC timestamp maintained at this node
    volatile static private HVC startPoint;  // the HVC timestamp when the system started being in current state
                                    // this should serve as start point of interval sent to monitor
                                    // startPoint is only updated when local state is changed to true
                                    // while procHvc is updated at any event/request
    static private long physicalStartTime; // the physical time counter-parter of startPoint

    // for managing interval
    static public final int PENDING_INTERVAL_STATUS_EMPTY = 0; // there is not pending interval
    static public final int PENDING_INTERVAL_STATUS_ACTIVE = 1; // there is a pending interval but not yet expired
    static public final int PENDING_INTERVAL_STATUS_EXPIRED = 2; // there is a pending interval and it has expired

    volatile static private boolean pendingIntervalFlag;
    volatile static private long pendingIntervalStartNs; // when the pending interval starts (in nanosecond)
                                                // this is, in theory the same as physicalStartTime (which is in millisecond)
                                                // but since Java nanoTime() and currentTimeMillis() use a different reference points
                                                // and we need resolution smaller than 1 ms for interval length, we will use this
                                                // This value is only meaningful when pendingIntervalFlag = True

    static private Table<String, MonitorVectorClock, String> localState;
    static private long candidateSeqNumber; // sequence number for candidates sent to monitors

    // static variables from parameters
    static private long epsilon; // synchronization error
    static private double beta;    // frequency of local predicate being true
    static private double alpha;   // frequency of communication
    static private int baseIntervalLength;   // how long interval is being true
    static private double geoDistParam;      // used when interval length follow geometric distribution
                                            // not used if we want fixed length
    static private long localStepComputationTimeMuS; // how long a step of computation takes (in micro-second)
    static private int numberOfProcesses;    // number of wcpprocess.process in computation
    static private int numberOfBins;         // for histogram
    static private int processBasePortNumber;    // each wcpprocess.process will open a port at processBasePortNumber + processId
    static private int runId;
    static private String monitorIpAddrStr;
    static private String monitorBasePortNumberStr;  // since we have one monitor, this is the port to communicate with monitor
    static private String experimentLength;

    static public String inputArgStr;

    volatile static private HashMap<Integer, String> machineAddressMap; // map nodeId to IP address

    // communication channels
    static public final int MAX_PROCESS_BUFFER_SIZE = 1024 * 2;

    // channels for receiving messages from other processes
    static private ConcurrentHashMap<String, ByteBuffer> incomingProcessChannelBufferMap;
    static private ConcurrentLinkedQueue<ProcessMessage> receivedMessagesQueue;

    // channels for sending messages to other processes
    static private ConcurrentHashMap<Integer, SocketChannel> outgoingProcessChannelMap;
    static private LinkedBlockingQueue<OutGoingMessageWrapper> outgoingMessagesQueue;

    static HashMap<String, String> monitorIdAddrMap; // monitorId, monitorIp:monitorPort
    static HashMap<String, SocketChannel> monitorIdConnectionMap; // monitorId, socketConnection


    // some statistics help computing alpha
    volatile static int messageSentCount;
    volatile static int messageReceivedCount;

    public static int getProcessId() {
        return processId;
    }

    public static long getEpsilon() {
        return epsilon;
    }

    public static double getBeta() {
        return beta;
    }

    public static double getAlpha() {
        return alpha;
    }

    public static int getBaseIntervalLength() {
        return baseIntervalLength;
    }

    public static double getGeoDistParam() {
        return geoDistParam;
    }

    public static long getLocalStepComputationTimeMuS() {
        return localStepComputationTimeMuS;
    }

    public static int getNumberOfProcesses() {
        return numberOfProcesses;
    }

    public static int getNumberOfBins() {
        return numberOfBins;
    }

    public static int getProcessBasePortNumber() {
        return processBasePortNumber;
    }

    public static int getRunId() {
        return runId;
    }

    public static String getMonitorIpAddrStr() {
        return monitorIpAddrStr;
    }

    public static String getMonitorBasePortNumberStr() {
        return monitorBasePortNumberStr;
    }

    public static HVC getProcHvc(){
        return procHvc;
    }

    public static HVC getStartPoint(){
        return startPoint;
    }

    public static ConcurrentLinkedQueue<ProcessMessage> getReceivedMessagesQueue(){
        return receivedMessagesQueue;
    }

    public static void setProcHvc(HVC hvc){
        procHvc = hvc;
    }

    // do not write setStartPoint like this because it will make startPoint and procHvc pointing to the same object
    // therefore startPoint and procHvc is always identical
//    public static void setStartPoint(HVC hvc){
//        startPoint = hvc;
//    }

    public static void setPhysicalStartTime(long pt){
        physicalStartTime = pt;
    }

    public static long getPhysicalStartTime(){
        return physicalStartTime;
    }

    public static void advanceStartPointToCurrentHvc(){
        startPoint.copyHVC(procHvc);

        setPhysicalStartTime(System.currentTimeMillis());
    }

    public static Table<String, MonitorVectorClock, String> getLocalState(){
        return localState;
    }

    public static long getCandidateSeqNumber(){
        return candidateSeqNumber;
    }

    public static void incrementCandidateSeqNumber(){
        candidateSeqNumber ++;
    }

    public static boolean getPendingIntervalFlag(){
        return pendingIntervalFlag;
    }

    public static void setPendingIntervalFlag(boolean status){
        pendingIntervalFlag = status;

        // record when pending interval starts
        if(status == true){
            setPendingIntervalStartNs(System.nanoTime());
        }
    }

    public static long getPendingIntervalStartNs(){
        return pendingIntervalStartNs;
    }

    public static void setPendingIntervalStartNs(long timestamp){
        pendingIntervalStartNs = timestamp;
    }


    public static void main(String[] args){
        // parse input argument
        OptionParser mainParser = new OptionParser();

        OptionSpec<Integer> processIdOptionSpec = mainParser.accepts("process-id")
                .withRequiredArg()
                .ofType(Integer.class);
        mainParser.accepts("argument-list-file").withRequiredArg();
        mainParser.accepts("machine-list-file").withRequiredArg();

        OptionSet mainOptionSet = mainParser.parse(args);
        processId = mainOptionSet.valueOf(processIdOptionSpec);
        String argListFileName = (String) mainOptionSet.valueOf("argument-list-file");
        String machineListFileName = (String) mainOptionSet.valueOf("machine-list-file");

//        System.out.println("argument-list-file = " + argListFileName);
//        System.out.println("machine-list-file = " + machineListFileName);
//        System.out.println("Working Directory = " +
//                System.getProperty("user.dir"));

        File argListFile = new File(argListFileName);
        File machineListFile = new File(machineListFileName);

        // reading machine list
        Vector<String> machineList = new Vector<>();
        try {
            BufferedReader reader = new BufferedReader(new FileReader(machineListFile));
            String aLine = reader.readLine();
            while(aLine != null){
                String[] lineSplit = aLine.split("@");
                machineList.addElement(lineSplit[1]);
                aLine = reader.readLine();
            }
        } catch(Exception e){
            log(" ERROR: when open and read file " + machineListFileName);
            log(e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }

        // read input arguments provided in the file argument_list_file
        String[] argList = null;
        try {
            BufferedReader reader = new BufferedReader(new FileReader(argListFile));

            argList = reader.readLine().split("\\s+");
        } catch(Exception e){
            log(" ERROR: when open and read file " + argListFileName);
            log(e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }

        System.out.println();
        for(int i = 0; i < argList.length; i ++){
            System.out.println("argList["+i+"] " + argList[i]);
        }
        System.out.println();

        OptionParser inputFileParser = new OptionParser();

        OptionSpec<Long> epsilonOptionSpec = inputFileParser.accepts("epsilon")
                .withRequiredArg()
                .ofType(Long.class);
        OptionSpec<Double> betaOptionSpec = inputFileParser.accepts("beta")
                .withRequiredArg()
                .ofType(Double.class);
        OptionSpec<Double> alphaOptionSpec = inputFileParser.accepts("alpha")
                .withRequiredArg()
                .ofType(Double.class);
        OptionSpec<Integer> baseIntervalLengthOptionSpec = inputFileParser.accepts("base-interval-length")
                .withRequiredArg()
                .ofType(Integer.class);
        OptionSpec<Double> geoDistParamOptionSpec = inputFileParser.accepts("geo-dist-param")
                .withRequiredArg()
                .ofType(Double.class);
        OptionSpec<Long> localStepComputationTimeOptionSpec = inputFileParser.accepts("local-step-computation-time")
                .withRequiredArg()
                .ofType(Long.class);
        OptionSpec<Integer> numberOfProcessesOptionSpec = inputFileParser.accepts("number-of-processes")
                .withRequiredArg()
                .ofType(Integer.class);
        OptionSpec<Integer> numberOfBinsOptionSpec = inputFileParser.accepts("number-of-bins")
                .withRequiredArg()
                .ofType(Integer.class);
        OptionSpec<Integer> processBasePortNumberOptionSpec = inputFileParser.accepts("process-base-port-number")
                .withRequiredArg()
                .ofType(Integer.class);
        OptionSpec<Integer> runIdOptionSpec = inputFileParser.accepts("run-id")
                .withRequiredArg()
                .ofType(Integer.class);
        inputFileParser.accepts("monitor-ip-addr").withRequiredArg();
        inputFileParser.accepts("monitor-base-port-number").withRequiredArg();
        inputFileParser.accepts("experiment-length").withRequiredArg();

        OptionSet inputFileOptions = inputFileParser.parse(argList);
        epsilon = inputFileOptions.valueOf(epsilonOptionSpec);
        beta = inputFileOptions.valueOf(betaOptionSpec);
        alpha = inputFileOptions.valueOf(alphaOptionSpec);
        baseIntervalLength = inputFileOptions.valueOf(baseIntervalLengthOptionSpec);
        geoDistParam = inputFileOptions.valueOf(baseIntervalLengthOptionSpec);
        localStepComputationTimeMuS = inputFileOptions.valueOf(localStepComputationTimeOptionSpec);
        numberOfProcesses = inputFileOptions.valueOf(numberOfProcessesOptionSpec);
        numberOfBins = inputFileOptions.valueOf(numberOfBinsOptionSpec);
        processBasePortNumber = inputFileOptions.valueOf(processBasePortNumberOptionSpec);
        runId = inputFileOptions.valueOf(runIdOptionSpec);
        monitorIpAddrStr = (String) inputFileOptions.valueOf("monitor-ip-addr");
        monitorBasePortNumberStr = (String) inputFileOptions.valueOf("monitor-base-port-number");
        experimentLength = (String) inputFileOptions.valueOf("experiment-length");

        inputArgStr =
                "epsilon= " + epsilon + "\n" +
                        "beta= " + beta  + "\n" +
                        "alpha=" + alpha  + "\n" +
                        "base-interval-length= " + baseIntervalLength  + "\n" +
                        "geo-dist-param= " + geoDistParam  + "\n" +
                        "local-step-computation-time= " + localStepComputationTimeMuS + "\n" +
                        "number-of-processes= " + numberOfProcesses  + "\n" +
                        "number-of-bins= " + numberOfBins  + "\n" +
                        "process-base-port-number= " + processBasePortNumber  + "\n" +
                        "monitor-ip-addr= " + monitorIpAddrStr  + "\n" +
                        "monitor-base-port-number= " + monitorBasePortNumberStr  + "\n" +
                        "run-id= " + runId  + "\n" +
                        "experiment-length= " + experimentLength + "\n";

        System.out.println(inputArgStr);

        machineAddressMap = new HashMap<>();
        for(int procId = 0; procId < numberOfProcesses; procId++){
            // note that multiple process may be running on same machine
            int machineIndex = procId % machineList.size();
            String machineIp = machineList.elementAt(machineIndex);
            machineAddressMap.put(procId, machineIp);
        }

        initProcess();

        stopThreadNow = false;

        // start threads for receiving and sending messages
        ProcessMessageReceiver processMessageReceiver =
                new ProcessMessageReceiver(processId,
                        processBasePortNumber,
                        receivedMessagesQueue,
                        incomingProcessChannelBufferMap);

        ProcessMessageSender processMessageSender = new ProcessMessageSender(outgoingMessagesQueue, machineAddressMap, outgoingProcessChannelMap);

        //WcpProcess applicationProcess = new WcpProcess();
        WcpProcessSplitInterval applicationProcess = new WcpProcessSplitInterval();

        new Thread(processMessageReceiver).start();
        new Thread(processMessageSender).start();
        new Thread(applicationProcess).start();

        // add shutdown hook
        Runtime.getRuntime().addShutdownHook(new Thread() {

            @Override
            public void run() {
                // let producer and consumer thread now that they should stop and do their clean up
                stopThreadNow = true;

                System.out.println(" Receive addShutdownHook signal");
                System.out.println(" Stoping processMessageReceiver threads");

                // write results to file with information of parameters
                stopTimeMs = System.currentTimeMillis();

                String process_data_file_name = "data_proc" + getProcessId()
                        + "_p" + numberOfProcesses
                        + "_a" + alpha
                        + "_e" + epsilon
                        + "_l" + beta
                        + "_ilen" + baseIntervalLength
                        + "_lsct" + localStepComputationTimeMuS
                        + "_elen" + experimentLength
                        + "_run" + runId;

                applicationProcess.writeResultsToFile(inputArgStr, process_data_file_name);

            }
        });

    }

    /**
     * send specific message to a specific node using TCP
     * The message is put in a blocking queue
     * and a separate thread will take care of sending the message to destination
     * This mechanism address the issue of sendMessageToDestinationFromMainThread in which
     * the message will be delivered in one iteration/step.
     * @param msg
     * @param nodeId
     */
    public static void sendMessageToDestinationThroughSeparateThread(ProcessMessage msg, int nodeId)
            throws IOException, InterruptedException {

        OutGoingMessageWrapper outGoingMessageWrapper = new OutGoingMessageWrapper(msg, nodeId);
        if(!outgoingMessagesQueue.offer(outGoingMessageWrapper)){
            // may be the queue is full
            CommonUtils.log(" Could not insert OutGoingMessageWrapper into queue." +
                            " Remaining capacity = " + outgoingMessagesQueue.remainingCapacity());
        }
    }

    /**
     * send specific message to a specific node using TCP
     * The message is sent from the main thread, not through the separate thread
     * This method has one issue: since it uses TCP, it will only return when the receiver confirm the
     * reception. Thus, the delivery of a message is done within one iteration/step.
     * This is different from the analytical model where message can be delivered in several steps.
     * In other word, delta = 1.
     * Although network latency (delta) does not affect the false positive rate, having small delta
     * will make interval size small since instant delivery of message will make interval split.
     * Since interval size small, it is difficult to investigate the impact of interval size on false positive rate.
     * Note that the graph of false positive rate does not change much
     * when interval size varies in small range, e.g. from 1 to 10.
     * @param msg
     * @param nodeId
     */
    public static void sendMessageToDestinationFromMainThread(ProcessMessage msg, int nodeId)
            throws IOException, InterruptedException {
        // obtain the socket for nodeId
        SocketChannel outgoingChannel;
        String nodeIpAddr = machineAddressMap.get(nodeId);
        int nodePortNumber = processBasePortNumber + nodeId;

        if(!outgoingProcessChannelMap.containsKey(nodeId)){
            // channel need to be created
            InetSocketAddress nodeSocketAddr = new InetSocketAddress(InetAddress.getByName(nodeIpAddr),
                    nodePortNumber);

            outgoingChannel = SocketChannel.open(nodeSocketAddr);

            // keep this connection alive
            outgoingChannel.socket().setKeepAlive(true);

            outgoingProcessChannelMap.put(nodeId, outgoingChannel);
        }else{
            // channel already created
            outgoingChannel = outgoingProcessChannelMap.get(nodeId);
        }

        // send message
        // it is possible the connection is closed by monitor, then we need to reopen it
        if(outgoingChannel.socket().isClosed()){

            log(" sendMessageToDestinationThroughSeparateThread: connection is already closed. Reopening it");

            // close
            outgoingChannel.close();

            // and reopen it
            InetSocketAddress nodeSocketAddr = new InetSocketAddress(InetAddress.getByName(nodeIpAddr),
                    nodePortNumber);

            outgoingChannel = SocketChannel.open(nodeSocketAddr);

            // keep this connection alive
            outgoingChannel.socket().setKeepAlive(true);
            outgoingProcessChannelMap.put(nodeId, outgoingChannel);

        }else{
//                log(" sendMessageToDestinationThroughSeparateThread: connection is still active");
        }



        // preparing byte array
        byte[] msgData = msg.toByteArray();
        int totalLength = msgData.length + ByteUtils.SIZE_OF_INT;
        byte[] backingArray = new byte[totalLength];
        int offset = 0;

        // write total length to backingArray
        ByteUtils.writeInt(backingArray, totalLength, offset);
        offset += ByteUtils.SIZE_OF_INT;

        // write message
        System.arraycopy(msgData, 0, backingArray, offset, totalLength - ByteUtils.SIZE_OF_INT);

        // send message out
        ByteBuffer buffer = ByteBuffer.wrap(backingArray);

        int numberOfBytesWritten;
        numberOfBytesWritten = outgoingChannel.write(buffer);

//        log("totalLength = " + totalLength + "  numberOfBytesWritten = " + numberOfBytesWritten);
//        log("sent (" + numberOfBytesWritten + ") from " + getProcessId() + " -> " + nodeId + ": " + msg.getMessageContent());


        messageSentCount ++;
    }


    private static void initProcess(){
        startTimeMs = System.currentTimeMillis();

        // init Hvc
        procHvc = new HVC( numberOfProcesses,
                epsilon,
                processId,
                System.currentTimeMillis());

        startPoint = new HVC( numberOfProcesses,
                epsilon,
                processId,
                System.currentTimeMillis());


        incomingProcessChannelBufferMap = new ConcurrentHashMap<>();
        receivedMessagesQueue = new ConcurrentLinkedQueue<>();
        outgoingProcessChannelMap = new ConcurrentHashMap<>();
        outgoingMessagesQueue = new LinkedBlockingQueue<>();

        messageSentCount = 0;
        messageReceivedCount = 0;

        // init local state
        localState = HashBasedTable.create();
        MonitorVectorClock dumpMvc = new MonitorVectorClock();
        localState.put("x_" + processId, dumpMvc, "0");

        setPendingIntervalFlag(false);

        // init sequence number for candidates
        candidateSeqNumber = 0L;

        // Init permanent connection to the monitor
        // Note that we have one monitor with ID 0
        monitorIdAddrMap = new HashMap<>();
        monitorIdConnectionMap = new HashMap<>();
        int monitorId = 0;
        String monitorIdStr = Integer.toString(monitorId);
        int monitorPortNumber = Integer.valueOf(monitorBasePortNumberStr) + monitorId;

        try {
            // open connection if it is new
            if(!monitorIdAddrMap.containsKey(monitorId)){
                InetSocketAddress monitorAddr = new InetSocketAddress(InetAddress.getByName(monitorIpAddrStr), monitorPortNumber);
                SocketChannel monitorClient = SocketChannel.open(monitorAddr);

                // keep this connection alive
                monitorClient.socket().setKeepAlive(true);

                monitorIdAddrMap.put(monitorIdStr, monitorIpAddrStr + ":" + monitorPortNumber);
                monitorIdConnectionMap.put(monitorIdStr, monitorClient);
            }
        }catch(Exception e) {
            log("initProcess() error: could not open connection to monitor");
            log(" monitorId = " + monitorId + " monitorAddr = " + monitorIpAddrStr + ":" + monitorPortNumber);
            log(e.getMessage());
        }
    }

    public static boolean isLocalStateTrue(){
        String varName = "x_" + getProcessId();
        String desiredValue = "1";

        Map<MonitorVectorClock, String> verValMap = getLocalState().row(varName);
        if(!verValMap.containsValue(desiredValue))
            return false;

        return true;
    }

    public static int getPendingIntervalStatus(){
        if(getPendingIntervalFlag() == false){
            return PENDING_INTERVAL_STATUS_EMPTY;
        }else{
            // there is a pending interval, we check if interval length has been reached or not
            // note the unit of time for interval length is microsecond
            long SCALE = 1000;  // one microsecond is 1000 nanosecond
            if((System.nanoTime() - getPendingIntervalStartNs()) >= getBaseIntervalLength()*SCALE){
                // pending interval expires
                return PENDING_INTERVAL_STATUS_EXPIRED;
            }else{
                // pending interval has not yet expired
                return PENDING_INTERVAL_STATUS_ACTIVE;
            }
        }
    }

    // send out the pending candidate
    // return   interval length of the candidate if successful (>=0)
    //                  it also marks pendinIntervalFlag as false
    //          -1 if there is not pending interval or if the sending is not successful
    public static long sendCandidateNow(){
        if(getPendingIntervalFlag() == false)
            return -1;

        // form candidate and send to monitor
        HvcInterval outgoingHvcInterval = new HvcInterval(getPhysicalStartTime(), getStartPoint(), getProcHvc());
        LocalSnapshotContent lsc = new LocalSnapshotContent(getLocalState());
        String predicateName = "0"; // assume we have predicate with ID 0
        incrementCandidateSeqNumber();

        LocalSnapshot outgoingLocalSnapshot =
                new LocalSnapshot(outgoingHvcInterval, getCandidateSeqNumber(), lsc);
        CandidateMessageContent outgoingCandidateMessageContent =
                new CandidateMessageContent(predicateName, outgoingLocalSnapshot);

        long intervalLengthMs;
        try {
            int monitorId = 0;
            sendCandidateToMonitor(outgoingCandidateMessageContent, monitorId);

            HvcElement endPoint = outgoingHvcInterval.getEndPoint().getPrimaryElement();
            HvcElement startPoint = outgoingHvcInterval.getStartPoint().getPrimaryElement();

            intervalLengthMs = endPoint.getTimeValueInMs() - startPoint.getTimeValueInMs();

//            totalCandidateCount ++;
//            totalCandidateIntervalLength += intervalLengthMs;

            // mark pendingIntervalStatus
            setPendingIntervalFlag(false);

        }catch(Exception e){
            log("changeLocalState: ERROR " + e.getMessage());
            e.printStackTrace();

            return -1;
        }

        return intervalLengthMs;
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

}