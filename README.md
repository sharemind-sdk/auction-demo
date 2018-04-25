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

## Getting the results out of Sharemind

## Conclusion
