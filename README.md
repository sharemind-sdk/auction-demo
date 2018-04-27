# Auction demo

This article will be about a small demonstration of using the Sharemind MPC
platform.

## Storyline

I chose a very simple problem that has 3 stakeholders: Alice, Bob and Charlie
(sorry for being so unimaginative with the names).

The story goes that Charlie wants to sell his house on the beach. Potential
buyers are Alice and Bob, who both are real-estate speculators who compete with
each-other. Charlie is interested in the highest bidder and wants to hold a
auction. Alice and Bob both want to make a bid, but since they are competing in
the same district they do not want to reveal their bids. So Charlie proposes to make
the auction closed.

So far Sharemind is still not needed: Alice and Bob can write their bids on a
piece of paper, put it into an envelope, close the envelope and give it to
Charlie, who will determine the winner. Charlie promises to not reveal the
losing bid. [Not every problem needs to be solved with Sharemind][Sharemind
problem].

However, Alice is not happy with that plan. She is afraid that if Bob wins the
auction and is going to celebrate the house deal with Charlie, it would be very
easy for Bob to persuade Charlie into giving him information about the amount
Alice was willing to pay. That information would give Bob an edge over Alice in
future deals. In short, Alice does not trust Charlie to keep the losing bid to
himself.

And now we have a problem, that is suitable for solving with Sharemind.

We need an application that allows Alice and Bob to enter their bids into the
Sharemind system and another application for Charlie to see the outcome of the
auction (winner and the amount of winning bid).

[Sharemind problem]: https://sharemind.cyber.ee/what-is-a-sharemind-problem/

## System architecture
I am not going to give you an [overview of Sharemind][Sharemind overview] in
general, but I will explain the setup for solving this particular problem. Each
stakeholder will host a Sharemind Application Server, and in addition, each will
host a Redis database

The base for the code comes from our [standalone application
demo][Standalone-demo], however that demo is not very good as there is only one
party that inputs data and also gets the results. In order to use multiple
inputs from separate stakeholders, we will need to use some kind of data storage
inside the Sharemind system. At the moment of this writing there are two ways
for storing data inside the system:
  1. mod\_tabledb -- a tabular storage format based on [HDF5].
  2. mod\_keydb -- an adapter to [Redis] key-value database.

I chose to keep the code simple and used mod\_keydb, although it requires each
stakeholder to also host a Redis server.

The data model is very simple, there are two variables named "a" and "b" in the
key-value database. The first holds the bid for Alice and the second, "b" holds
the amount for Bob.

[Sharemind overview]: https://sharemind.cyber.ee/homomorphic-encryption/
[Standalone-demo]: https://github.com/sharemind-sdk/standalone-demo
[HDF5]: https://portal.hdfgroup.org/display/HDF5/HDF5
[Redis]: https://redis.io

## Sharemind setup

### Key generation

### Access control

## Entering bids into Sharemind
### Server side
We start with the SecreC [code][Alicebid] to enter the bid for Alice.

```c
import shared3p;
import keydb;
import shared3p_keydb;

domain pd_shared3p shared3p;

void main() {
    keydb_connect("dbhost");

    pd_shared3p uint b = argument("bid");
    keydb_set("a", b);

    keydb_disconnect();
}
```
At the start we have some imports, which I will explain:
  * shared3p -- the definition of the shared3p protection domain kind.
  * keydb -- for the keydb functions using public data (in our case
    `keydb_connect(string)` and `keydb_close()`).
  * shared3p_keydb -- for the keydb functions specialized for shared3p
    protection domain (`keydb_set`).

Then we have the definition of the security domain `pd_shared3p` of kind
`shared3p`. In general, you can have multiple security domains with different
kinds, but for now you can mostly ignore that, because officially we offer only
one protection domain kind: `shared3p`.

We have the main entry point of the program and as people used to C, this is
called `main`, however unlike C the arguments to the SecreC program are not
given as parameters of `main`, but they can be obtained using the
`argument(string)` function.

At first we establish connection to the Redis server by using the
`keydb_connect(string)` function. Note that the name *dbhost* must match the
name in the mod\_keydb configuration file.

Next we declare a private variable `b` of security domain `pd_shared3p` and type
`uint` that is short for `uint64`, which is a 64-bit unsigned integer. In the
same line we assign the variable with the argument named "bid" that is gotten
from the runner of the SecreC program.

Then we store the private data to the Redis database under the key "a", that
means we store it as Alice's bid.

In the end we close the connection to Redis, but this is actually not needed in
this specific example, because the program is finished and the resources get
freed anyway.

For Bob the [file][Bobbid] is almost identical, the only difference is:
```
-     keydb_set("a", b);
+     keydb_set("b", b);
```
Instead of storing to variable "a" we store the bid to "b", which corresponds to
Bob's bid.

[Alicebid]: https://github.com/sharemind-sdk/url/to/file
[Bobbid]: https://github.com/sharemind-sdk/url/to/file

### Client side

The full [file][bid.cpp] starts with argument parsing using Boost
program\_options. After that we load the Sharemind Controller configuration with
the following lines:
```C++
if (vm.count("conf")) {
    config = sm::makeUnique<sm::SystemControllerConfiguration>(
                 vm["conf"].as<std::string>());
} else {
    config = sm::makeUnique<sm::SystemControllerConfiguration>();
}
```
If we have a `"conf"` options specified we give that to the constructor to load the
configuration from that file, otherwise the client configuration is searched
from the default locations.

Next we need to set up an `LogHard::Logger` instance:

```C++
auto logBackend(std::make_shared<LogHard::Backend>());
logBackend->addAppender(std::make_shared<LogHard::StdAppender>());
logBackend->addAppender(
            std::make_shared<LogHard::FileAppender>(
                "auction-bid.log",
                LogHard::FileAppender::OVERWRITE));
const LogHard::Logger logger(logBackend);
```

After that is done we can create an instance of the `SystemController`, create
an `IController::ValueMap` instance for the arguments and add an argument called
"bid", that the SecreC program expects. Note that the `IController::Value`
constructor also needs the protection domain and type of the argument. The third
argument to `Value` constructor is of type `std::shared_ptr<void>` and the data
is used directly as a pointer. Therefore we also need to pass in the size of the
data. For passing in arrays of data, we would point to the start of the array
and give the size as `sizeof(sm::Uint64) * lengthOfArray`.

```C++
sm::SystemControllerGlobals systemControllerGlobals;
sm::SystemController c(logger, *config);

// Initialize the argument map and set the arguments
sm::IController::ValueMap arguments;

arguments["bid"] =
    std::make_shared<sm::IController::Value>(
        "pd_shared3p",
        "uint64",
        std::make_shared<sm::UInt64>(bid),
        sizeof(sm::UInt64));
```
After we are finished with the arguments we just run the right bytecode program
with the arguments and as a result we will get back the same kind of
`IController::ValueMap` that we used for the arguments:
```C++
sm::IController::ValueMap results = c.runCode(bytecode[bidder], arguments);
```
Note that `bytecode[bidder]` evaluates to `"alice_bid.sb"` or `"bob_bid.sb"`
depending on the command line arguments.

[bid.cpp]: https://github.com/sharemind-sdk/url/to/file

## Getting the results out of Sharemind
### Server side

For getting the results the [code][result.sc] is straightforward:
```C
import shared3p;
import keydb;
import shared3p_keydb;
import oblivious;

domain pd_shared3p shared3p;

void main() {
    keydb_connect("dbhost");

    pd_shared3p uint a = keydb_get("a");
    pd_shared3p uint b = keydb_get("b");

    pd_shared3p bool aliceWon = a > b;
    pd_shared3p uint winningBid = choose(aliceWon, a, b);

    publish("aliceWon", aliceWon);
    publish("winningBid", winningBid);

    keydb_disconnect();
}
```
We get the Alice's bid `"a"` and Bob's bid `"b"` from the database and we
compare them. The `choose` function is a private variant of the C's ternary
operator (`<cond> ? <true_expr> : <false_expr>`). Note that in this case we
cannot use the typical `if/else` because that would need a public boolean. The
`choose` function is defined in the `oblivious` module.

For outputting something from a SecreC program, one has to use the `publish`
function, which sends the secret shared output back to the client.

[result.sc]: https://github.com/sharemind-sdk/url/to/file

### Client side
The [C++ code][result.cpp] for getting the result is mostly the same as for
entering the bid. Just that now we give no arguments to the SecreC program and
we will get back the published results.

```C++
sm::SystemControllerGlobals systemControllerGlobals;
sm::SystemController c(logger, *config);

// Initialize the argument map and set the arguments
sm::IController::ValueMap arguments;

// Run code
logger.info() << "Running SecreC bytecode on the servers.";
sm::IController::ValueMap results = c.runCode("charlie_result.sb", arguments);

// Print the result
auto aliceWon = results["aliceWon"]->getValue<sm::Bool>();
auto winningBid = results["winningBid"]->getValue<sm::UInt64>();

logger.info() << "The winner is "
    << (aliceWon ? "Alice" : "Bob")
    << ". With a bid of "
    << winningBid
    << ".";
```

[result.cpp]: https://github.com/sharemind-sdk/url/to/file

## Demonstration
### Alice enters the bid
```
$ ./auction-bid -a --bid 1000
2018.04.26 17:34:50 INFO    [Controller] Loaded controller module 'shared3p' (15 data types, 1 PDKs) from 'libsharemind_mod_shared3p_ctrl.so' using API version 1.
2018.04.26 17:34:50 INFO    [Controller] Started protection domain 'pd_shared3p' of kind 'shared3p'.
2018.04.26 17:34:50 INFO    Sending secret shared arguments and running SecreC bytecode on the servers
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Connecting...
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Connecting...
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Connected. Authenticating...
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Connected. Authenticating...
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Connecting...
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Connected. Authenticating...
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Authenticated.
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Authenticated.
2018.04.26 17:34:50 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Authenticated.
2018.04.26 17:34:50 INFO    [Controller] Stopping the network...
```

### Bob enters the bid
```
$ ./auction-bid -b --bid 1100
2018.04.26 17:35:52 INFO    [Controller] Loaded controller module 'shared3p' (15 data types, 1 PDKs) from 'libsharemind_mod_shared3p_ctrl.so' using API version 1.
2018.04.26 17:35:52 INFO    [Controller] Started protection domain 'pd_shared3p' of kind 'shared3p'.
2018.04.26 17:35:52 INFO    Sending secret shared arguments and running SecreC bytecode on the servers
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Connecting...
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Connecting...
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Connecting...
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Connected. Authenticating...
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Connected. Authenticating...
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Connected. Authenticating...
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Authenticated.
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Authenticated.
2018.04.26 17:35:52 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Authenticated.
2018.04.26 17:35:52 INFO    [Controller] Stopping the network...
```

### Charlie gets the result
```
$ ./auction-result
2018.04.26 17:36:20 INFO    [Controller] Loaded controller module 'shared3p' (15 data types, 1 PDKs) from 'libsharemind_mod_shared3p_ctrl.so' using API version 1.
2018.04.26 17:36:20 INFO    [Controller] Started protection domain 'pd_shared3p' of kind 'shared3p'.
2018.04.26 17:36:20 INFO    Running SecreC bytecode on the servers.
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Connecting...
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Connecting...
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Connecting...
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Connected. Authenticating...
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Connected. Authenticating...
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Connected. Authenticating...
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner1] To 127.0.0.1:30001: Authenticated.
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner3] To 127.0.0.1:30003: Authenticated.
2018.04.26 17:36:20 INFO    [Controller][Server DebugMiner2] To 127.0.0.1:30002: Authenticated.
2018.04.26 17:36:20 INFO    The winner is Bob. With a bid of 1100.
2018.04.26 17:36:20 INFO    [Controller] Stopping the network...
```

## Running the examples with the Sharemind Emulator
For running the SecreC programs with the Sharemind Emulator one needs to know
the protocol for giving arguments and receiving the results. The argument stream protocol is
described in a [man page][emulator protocol].

Alice part:
```
$ sharemind-emulator alice_bid.sb --str=bid --str=pd_shared3p --str=uint64 --size=8 --uint64=1000
```

Bob:
```
$ sharemind-emulator bob_bid.sb --str=bid --str=pd_shared3p --str=uint64 --size=8 --uint64=1100
```
Charlie:
```
$ sharemind-emulator charlie_result.sb | xxd
Process returned status: 0
00000000: 0800 0000 0000 0000 616c 6963 6557 6f6e  ........aliceWon
00000010: 0b00 0000 0000 0000 7064 5f73 6861 7265  ........pd_share
00000020: 6433 7004 0000 0000 0000 0062 6f6f 6c01  d3p........bool.
00000030: 0000 0000 0000 0000 0a00 0000 0000 0000  ................
00000040: 7769 6e6e 696e 6742 6964 0b00 0000 0000  winningBid......
00000050: 0000 7064 5f73 6861 7265 6433 7006 0000  ..pd_shared3p...
00000060: 0000 0000 0075 696e 7436 3408 0000 0000  .....uint64.....
00000070: 0000 004c 0400 0000 0000 00              ...L.......
```

I used xxd to show the raw bytes of the output. However, there are better tools
available for deciphering the raw argument stream protocol. (TODO give examples, find those tools)

[emulator protocol]: https://github.com/sharemind-sdk/emulator/blob/master/doc/sharemind-emulator.h2m

## Conclusion
