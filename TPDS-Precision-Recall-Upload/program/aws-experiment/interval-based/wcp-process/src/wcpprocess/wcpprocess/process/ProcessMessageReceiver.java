package wcpprocess.process;

import com.google.protobuf.InvalidProtocolBufferException;
import predicatedetectionlib.common.ByteUtils;
import predicatedetectionlib.common.CommonUtils;
import wcpprocess.protobuf.WcpMessageProtos.*;

import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.util.Iterator;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

import static predicatedetectionlib.common.CommonUtils.*;
import static predicatedetectionlib.monitor.MultiplePredicateCandidateReceiving.readDataFromChannelIntoBuffer;
import static wcpprocess.process.BasicProcess.MAX_PROCESS_BUFFER_SIZE;

/**
 * This thread will open an incoming port for receiving TCP messages from other processes
 * It also receives the messages and place them into a buffer
 */
public class ProcessMessageReceiver implements Runnable {
    int processId;
    int processBasePortNumber;
    ConcurrentLinkedQueue<ProcessMessage> receivedMessagesQueue;
    ConcurrentHashMap<String, ByteBuffer> incomingProcessChannelBufferMap;

    ProcessMessageReceiver(int processId,
                           int processBasePortNumber,
                           ConcurrentLinkedQueue<ProcessMessage> receivedMessagesQueue,
                           ConcurrentHashMap<String, ByteBuffer> incomingProcessChannelBufferMap){
        this.processId = processId;
        this.processBasePortNumber = processBasePortNumber;
        this.receivedMessagesQueue = receivedMessagesQueue;
        this.incomingProcessChannelBufferMap = incomingProcessChannelBufferMap;
    }

    public void run(){
        try {
            // Selector: multiplexer of SelectableChannel objects
            Selector selector = Selector.open(); // selector is open

            // ServerSocketChannel: selectable channel for stream-oriented listening sockets
            ServerSocketChannel procMsgIncomingSocket = ServerSocketChannel.open();
            InetSocketAddress processSocketAddr = new InetSocketAddress(getLocalNonLoopbackAddress(),
                                                                processBasePortNumber + processId);

            // Binds the channel's socket to a local address and configures the socket to listen for connections
            procMsgIncomingSocket.bind(processSocketAddr);

            // Adjusts this channel's blocking mode to non-blocking
            procMsgIncomingSocket.configureBlocking(false);

            // set timeout so that it won't close too early
            // this is important so that the connection between processes is persistent
            procMsgIncomingSocket.socket().setSoTimeout(0);

            int ops = procMsgIncomingSocket.validOps();
            SelectionKey selectKey = procMsgIncomingSocket.register(selector, ops, null);

            CommonUtils.log("\n");
            CommonUtils.log("ProcessMessageReceiver:");
            CommonUtils.log("               processId = " + processId);
            CommonUtils.log("   processBasePortNumber = " + processBasePortNumber);
            CommonUtils.log(" waiting for msg address = " + processSocketAddr.getAddress() + ":" + processSocketAddr.getPort());
            CommonUtils.log("\n");

            // Keep server running until it receive SIGINT signal

            while (!BasicProcess.stopThreadNow) {

                // Selects a set of keys whose corresponding channels are ready for I/O operations
                selector.select();

                // token representing the registration of a SelectableChannel with a Selector
                Set<SelectionKey> selectedKeys = selector.selectedKeys();
                Iterator<SelectionKey> keyIterator = selectedKeys.iterator();

                while (keyIterator.hasNext()) {
                    SelectionKey currentKey = keyIterator.next();

                    // Tests whether this key's channel is ready to accept a new socket connection
                    if (currentKey.isAcceptable()) {
                        SocketChannel procMsgIncomingChannel = procMsgIncomingSocket.accept();

                        // try to make socket not close
                        procMsgIncomingChannel.socket().setSoTimeout(0);

                        // Adjusts this channel's blocking mode to false
                        procMsgIncomingChannel.configureBlocking(false);

                        // Operation-set bit for read operations
                        procMsgIncomingChannel.register(selector, SelectionKey.OP_READ);

                        CommonUtils.log("Connection Accepted: " + procMsgIncomingChannel.getLocalAddress() + "\n");

                        // create a byte buffer and associate with this channel if this is new.
                        // Do not override old map since byte buffer in old mapping could contain data
                        String procMsgIncomingChannelStr = getStringRepresentSocketChannel(procMsgIncomingChannel);
                        if(! incomingProcessChannelBufferMap.containsKey(procMsgIncomingChannelStr)) {
                            ByteBuffer byteBuffer = ByteBuffer.allocate(MAX_PROCESS_BUFFER_SIZE);
                            incomingProcessChannelBufferMap.put(procMsgIncomingChannelStr, byteBuffer);
                        }

                        // Tests whether this key's channel is ready for reading
                    } else if (currentKey.isReadable()) {

                        SocketChannel voldServer = (SocketChannel) currentKey.channel();

                        // obtain associated byte buffer
                        ByteBuffer byteBuffer = incomingProcessChannelBufferMap.get(getStringRepresentSocketChannel(voldServer));

                        // read data stream from channel into buffer
                        // remember to change state of buffer after finish
                        readDataFromChannelIntoBuffer(voldServer, byteBuffer);

                        // extract processMessage from byte buffer
                        extractProcessMessagesFromBufferIntoQueue(byteBuffer);

                    }

                    keyIterator.remove();
                }
            }

            System.out.println("ProcessMessageReceiver thread ended");

        }catch(Exception e){
            CommonUtils.log(e.getMessage());
            e.printStackTrace();
            throw new RuntimeException();
        }

    }

    /**
     * Extract as many packet out of buffer as possible
     * Then compact the buffer so that we can read new data into the buffer
     * When the function begins, position should be 0
     * When the function finishes, position should be the last byte available
     * @param byteBuffer
     */
    private void extractProcessMessagesFromBufferIntoQueue(ByteBuffer byteBuffer){
        // when this function is called, byte buffer is ready for being read
        // i.e. position should point to the last byte written
        ProcessMessage procMsg;
        while((procMsg = extractProcessMessageFromByteBuffer(byteBuffer)) != null){
            // put procMsg into queue
            receivedMessagesQueue.add(procMsg);
        }

        // compact buffer for further reading new data
        byteBuffer.compact();

    }

    /**
     * Extract a ProcessMessage packet from buffer if possible
     * @param byteBuffer
     * @return a ProcessMessage if available, position of buffer should be advanced
     *         null if data is not complete for a ProcessMessage, position should be unchanged
     */
    private ProcessMessage extractProcessMessageFromByteBuffer(ByteBuffer byteBuffer){
        // mark current position
        byteBuffer.mark();

        // probe if a CandidateMessageContent is available
        if(! quickProbeByteBuffer(byteBuffer)){
            // restore value of position
            byteBuffer.reset();

            // return null indicating data has not fully arrived
            return null;
        }

        // restore value position for extraction
        byteBuffer.reset();

        // extract ProcessMessage from byte buffer
//        int remainingBytes = byteBuffer.remaining();

        int positionBeforeExtract = byteBuffer.position();
        int offset = positionBeforeExtract;
        byte[] backingArray = byteBuffer.array();

        // read the message length
        int msgLength = ByteUtils.readInt(backingArray, offset);
        offset += ByteUtils.SIZE_OF_INT;
        byte[] completeProcessMessageArray = new byte[msgLength - ByteUtils.SIZE_OF_INT];

        // copy data data from byte buffer into a separate bytebuffer
        System.arraycopy(backingArray, offset, completeProcessMessageArray, 0, msgLength - ByteUtils.SIZE_OF_INT);


        // update buffer position
        offset += (msgLength - ByteUtils.SIZE_OF_INT);
        byteBuffer.position(offset);

        // construct message
        ProcessMessage extractedProcMsg = null;
        try{
            extractedProcMsg = ProcessMessage.parseFrom(completeProcessMessageArray);

        }catch(InvalidProtocolBufferException ioe){
            System.out.println(" extractProcessMessageFromByteBuffer: ERROR: " + ioe.getMessage());
        }

//        CommonUtils.log("\n extractCandidateMessageContentFromByteBuffer: extract CMC from " + numOfBytes +
//                " out of " + remainingBytes + " bytes." +
//                " Old pos = " + positionBeforeExtract + " new pos = " + byteBuffer.position());

        return extractedProcMsg;
    }



}
