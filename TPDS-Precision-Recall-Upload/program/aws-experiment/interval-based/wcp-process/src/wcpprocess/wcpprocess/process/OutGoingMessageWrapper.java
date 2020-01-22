package wcpprocess.process;

import wcpprocess.protobuf.WcpMessageProtos.*;

/**
 * Created by duongnn on 11/25/18.
 * Encapsulation of outgoing message and destination
 */

public class OutGoingMessageWrapper {
    private ProcessMessage processMessage;
    private int destinationNodeId;

    public OutGoingMessageWrapper(ProcessMessage processMessage, int destinationNodeId) {
        this.processMessage = processMessage;
        this.destinationNodeId = destinationNodeId;
    }

    public ProcessMessage getProcessMessage() {
        return processMessage;
    }

    public int getDestinationNodeId() {
        return destinationNodeId;

    }
}