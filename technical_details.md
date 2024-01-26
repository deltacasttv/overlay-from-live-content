
# Sequence

The application performs the following actions:

1. Opens a handle on the specified device
2. Prints information related to that device
3. Determines whether the device is suitable
   - Does it have keying capabilities?
   - Is it an SDI device?
   - If not suitable, the application stops
4. Waits for an incoming signal on the specified input port
5. Configures the genlock based on the detected signal information and waits until the device is correctly locked onto the incoming signal
6. If overlay is enabled, configures the keyer
7. Creates, configures and starts an RX stream (in its own thread)
8. Creates, configures and starts a TX stream (in its own thread)
9. Disables the loopback
10. Waits until user input
11. Upon stop request, enables the loopback, stops the streams and exits

# VideoMaster Configuration

## Genlock

- `Source`: Input ID
- `Video standard`: Whatever is detected on input
- `Clock Divisor`: Whatever is detected on input

## Keyer

- `Input A`: Input ID
- `Input B`: Output ID
- `Input K`: Output ID
- `Video Output`: Keyer
- `ANC Output`: Input ID
- `Alpha Clip`: min 0, max 1020
- `Alpha Blend Factor`: 1023

## RX

- `Unconstrained` transfer scheme
- RGB 8b `buffer packing`

## TX

- `Preload`: 0
- `Buffer queue depth`: 16
- RGBA 8b `buffer packing` if overlay, RGB 8b if not
- `Genlocked`

# Data exchange

There needs to be some communication between the RX and TX threads to exchange data that is received so that it can be processed and transmitted.

In order to avoid too many copies, application buffer mode is used which allow the application to provide its own buffers to the device.
Furthermore, all buffers are allocated at the beginning of the application and are then reused throughout the application lifetime.

Two queues are used to exchange data between the RX and TX threads:
- A queue of TX buffers that are ready for processing
- A queue of fixed size of 1 for the most recently processed buffer that can be transmitted

This communication happens as follows:

1. The RX thread waits for a new buffer to be ready.
2. The RX thread pops a buffer from the queue of TX buffers that are marked as ready for processing.
3. The RX thread spawns a new processing thread that processes the buffer and marks it as ready for transmission when processing has finished. The RX buffer is then pushed back into the queue of the RX stream.
5. The TX thread periodically assesses that the on-board buffering is empty and when it is, acquires the buffer ready for transmission and pushes it into the queue of the TX stream.
6. The TX thread waits for the transfer to complete and then pushes the buffer back into the queue of buffers ready for processing.

Loop back to point `1`

# Minimal Latency

The minimal latency between input and output is 2 frames (should the processing be fast enough, see section `Details on the frame-based video interfacing` of https://www.deltacast.tv/technologies/low-latency).
In order to guarantee that in all cases, the following strategies have been implemented:

## RX

The buffer queue is emptied without notifying the TX thread before waiting for a new buffer.
That way, we can guarantee that the buffer that will be communicated to the TX thread is always the most recent one.

## TX

The RX thread is responsible for starting the processing in a dedicated thread and for updating the next buffer ready for transmission.

The only responsibility of the TX thread is to ensure that no buffering is performed on the device.

To that aim, it assesses prior to pushing the buffer into its queue that the latter is empty, otherwise the thread would not be able to accurately control the transfers and therefore the resulting end-to-end latency.

Note that therotecically, the latency that can be achieved is of the form `1 + ((processing_time + transfer_time_host_to_device + transfer_time_device_to_host) / period_of_the_signal)`, rounded up.

For instance,
```
processing_time = 9 ms
transfer_time_host_to_device = 12 ms
transfer_time_device_to_host = 14 ms
period_of_the_signal = 16.67 ms
```
leads to a latency of 4 frames, since `1 + ((9 + 12 + 14) / 16.67)=3.1`, rounded up and giving `4`.