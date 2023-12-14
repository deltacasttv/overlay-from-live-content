
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
- `Buffer queue depth`: 2
- RGBA 8b `buffer packing` if overlay, RGB 8b if not
- `Genlocked`

# Data exchange

There needs to be some communication between the RX and TX threads to exchange data that is received so that it can be processed and transmitted.

This communication happens as follows:

1. The TX thread waits until a buffer is declared `ready_to_process`
2. The RX thread waits for an incoming buffer
3. Once received, the RX thread communicates the pointer to the buffer to the TX thread and marks the buffer `ready_to_process`
4. The RX thread waits until the buffer is declared `processed`
5. The TX thread awakens, processes the buffer and transmits the data before marking the buffer `processed`
6. The RX thread awakens and releases its buffer

Loop back to point `1`

# Minimal Latency

The minimal latency between input and output is 2 frames (should the processing be fast enough, see section `Details on the frame-based video interfacing` of https://www.deltacast.tv/technologies/low-latency).
In order to guarantee that in all cases, the following strategies have been implemented:

## RX

The buffer queue is emptied without notifying the TX thread before waiting for a new buffer.
That way, we can guarantee that the buffer that will be communicated to the TX thread is always the most recent one.

## TX

Due to the nature of the communication between the RX and TX thread, the TX buffer queue will never be filled with more than one buffer at a time.
Indeed, since the RX thread is the one that drives the TX thread, we can never end up in a situation where the RX thread will give buffers to the TX thread faster that the rate at which they are consumed.

The only piece of buffer that needs to be controlled is the on-board buffer queue (typically of size 2).
When the TX thread awakens due to some buffer being ready to be processed, it checks the on-board buffer queue filling.
If it is greater than 1 (0 is impossible since the device is always processing a buffer) then the buffer needs to be skipped and we need to wait of the next frame, so that the device can consume the on-board buffer and go back to a queue of 1.
Waiting for that next video frame is achieved thanks to the cadencing of the RX thread.
The TX thread simply notifies that the processing has finished (although it actually wants to skip it) and then immediately waits again for a new buffer.
This has the effect that the RX thread will go through one full cycle again, thus waiting for the next frame and achieving the desired cadencing.