package wcpprocess.process;

import predicatedetectionlib.common.ByteUtils;
import predicatedetectionlib.common.CommonUtils;
import wcpprocess.protobuf.WcpMessageProtos.*;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import static predicatedetectionlib.common.CommonUtils.log;
import static wcpprocess.process.BasicProcess.*;

/**
 * Created by duongnn on 11/25/18
 * This thread is responsible for sending out messages to destinations
 * while not delaying the main thread.
 */
public class ProcessMessageSender implements Runnable {

    private LinkedBlockingQueue<OutGoingMessageWrapper> outgoingMessagesQueue;
    private HashMap<Integer, String> machineAddressMap;
    private ConcurrentHashMap<Integer, SocketChannel> outgoingProcessChannelMap;

    private final long BLOCKING_WAIT_TIME_MuS= 100L;    // this should be small so that it will not effect the latency much
                                                        // should not be too small to avoid high overhead
                                                        // unit: Microsecond

    ProcessMessageSender(LinkedBlockingQueue<OutGoingMessageWrapper> outgoingMessagesQueue,
                         HashMap<Integer, String> machineAddressMap,
                         ConcurrentHashMap<Integer, SocketChannel> outgoingProcessChannelMap){
        this.outgoingMessagesQueue = outgoingMessagesQueue;
        this.machineAddressMap = machineAddressMap;
        this.outgoingProcessChannelMap = outgoingProcessChannelMap;
    }

    public void run(){
        try{
            // preparation

            while(!BasicProcess.stopThreadNow){
                OutGoingMessageWrapper outGoingMessage;

                // wait for next message until available
                while((outGoingMessage = outgoingMessagesQueue.poll(BLOCKING_WAIT_TIME_MuS, TimeUnit.MICROSECONDS)) == null);

                // extract message and destination
                ProcessMessage msg = outGoingMessage.getProcessMessage();
                int nodeId = outGoingMessage.getDestinationNodeId();

                // send message out
                try{
                    // obtain the socket for nodeId
                    SocketChannel outgoingChannel;
                    String nodeIpAddr = machineAddressMap.get(nodeId);
                    int nodePortNumber = getProcessBasePortNumber() + nodeId;

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

                        log(" ProcessMessageSender: connection to node " + nodeId + " is already closed. Reopening it");

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
//                        log(" ProcessMessageSender: connection is still active");
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

                }catch(IOException ioe){
                    CommonUtils.log(" ProcessMessageSender: Exception when sending out message to node " + nodeId);
                    CommonUtils.log(ioe.getMessage());
//                    ioe.printStackTrace();
                }
            }

            System.out.println("ProcessMessageSender thread ended");

        }catch(Exception e){
            CommonUtils.log(e.getMessage());
            e.printStackTrace();
            throw new RuntimeException();
        }
    }
}
