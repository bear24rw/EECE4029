Producers & Consumers
=====================

Multiple Consumers
------------------
In order to support multiple consumers a doubly linked list is used. Each time
a call to `stream_connect()` is made a new `producer_t` struct is added to the
linked list. The `producer_t` struct contains a pointer to the producer stream
and pointers to the next and previous `producer_t` structs in the list.
Basically, each stream keeps a list of producer streams it consumes from.
Using this method it is extremely easy to connect any number of streams in any
combination. For example to connect two consumer streams to a successor:

```C
stream_t successor;
stream_t consumer_1;
stream_t consumer_2;

init_steam(&successor, NULL);
init_steam(&consumer_1, NULL);
init_steam(&consumer_2, NULL);

stream_connect(&consumer_1, &successor);
stream_connect(&consumer_2, &successor);
```

The `producer_t` struct also contains the field `buffer_idx` which indicates
the index into the producers buffer that the consumer is reading from. In
essence, each stream contains a list of all the producers it consumes from and
for each one of those producers it keeps track of which index in the producers
buffer it is currently at.

The `stream_t` struct also contains a field called `buffer_read_count[]` which
is the same length as the data buffer. Every time a new value is put into the
buffer by the `put()` function it resets the value in `buffer_read_count[]` at
the same index as the data value that was just written. Then, each time a
stream calls `get()` that value in `buffer_read_count[]` is incremented. It's
with this mechanism that the producer can ensure each thread has seen the value
before writing over it with the next one.

Dynamic Connect & Disconnect
----------------------------
By using a doubly linked list, as stated above, it is easy to connect and
disconnect individual connections. When a stream is connected to a producer
with `stream_connect` the pointer to the producer stream is simply added to the
end of the list. The starting `buffer_idx` is set to the producers `put_idx`
which is the next position the producer will write to. This ensures that newly
connected streams will only see fresh tokens. Also, the producers `num_consumers`
count is incremented so that it can keep track of how many consumers to wait for
before overwriting old values in the buffer.

When a stream is disconnected with `stream_disconnect` the list of producers
must be traversed in order to find which one we want to disconnect from.  Once
that producer has been identified the previous producers `next` pointer is set
to our `next` pointer, essentially taking ourselves out of the list. The same
is done from the `prev` pointer but in the opposite direction. We then
decrement `num_consumers` in the producers stream so that it can properly keep
track of how many consumers are still connected. We then loop through the
producers `buffer_read_count` array and decrement any value that is not zero to
prevent the `get` or `put` functions from waiting for us to read, which will
never happen now that we're disconnected.

A unit test showing this in action can be found in `tests/disconnect_reconnect.c`

Unit Tests
---------
In order to test the multitude of configurations that can be setup a series of
unit tests were created. Each test sets up and runs a different stream
configuration such as one producer to many consumers or many consumers to one
producer. Having a series of tests that could be quickly build and verify
against proved extremely useful when trying to ensure that a fix for one
situation didn't break another. The unit tests can be found in the `tests/`
folder and can be built in batch with `make tests`.

Timestamped Prints
-------------------
Early on it was determined that the `printf`'s from different threads would show
up out of order. This made is difficult to debug sequence sensitive events. To
solve this problem a `tprintf` macro was created:

```C
#define tprintf(...)    { \
    gettimeofday(&tv, NULL); \
    pthread_mutex_lock(&print_lock); \
    printf("%ld\t", tv.tv_sec*1000000+tv.tv_usec); \
    printf(__VA_ARGS__); \
    pthread_mutex_unlock(&print_lock); \
}
```

The macro assumes that the struct `timeval tv` is declared in the calling
function but that is all that is needed to utilize it. When it is called
it records the current time and then waits for a global `print_lock` mutex
to ensure that two printfs wont smash each other. It then prints out the
recorded timestamp in microseconds followed by whatever the original printf
arguments were. This allows us to now pipe our output through `sort -n` and
recreate the proper sequence of prints. This was enormously helpful in debugging
as the out of order prints can be deceiving sometimes.

Main Test Configuration
-----------------------
The main goal for this project was to convert the following stream flow

```
                       7,14,21,28...                1,2,3,4...
                              /--- Times 7 <---- successor
               5,7,10,14...  /
   consumer <--- merge <----<
                             \
                              \--- Times 5 <---- successor
                       5,10,15,20...              1,2,3,4...

```
to one that uses only one `successor` to feed both `Times` threads.

```
                       7,14,21,28...
                              /--- Times 7 <--\
               5,7,10,14...  /                 \       1,2,3,4...
   consumer <--- merge <----<                   ---- successor
                             \                 /
                              \--- Times 5 <--/
                       5,10,15,20...
```
This is done in `main.c` and is the default `make` build target.

GUI
---
An attempt at creating a GUI similar to the provided Java example was made
using GTK. Unfortunately it is only partially functional due to lack of time.
The GUI can be built using `make gui`.

![gui](https://raw.github.com/bear24rw/EECE4029/master/hw3_pthreads/gui/gui.png "GUI")
