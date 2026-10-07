/** Publisher, accumulators and stream adapters for the cross-DLL experiment
 *
 * Use when: tracing the participants introduced by main.cpp.
 * Demonstrates: publishers/subscribers for float and int plus their serialization adapters.
 * Story: ReadingSource emits 1.019F and 2 per call. ReadingAccumulator adds both to total;
 * ReadingStreamForwarder writes serialized bytes to stdout via its inherited serializer.
 * ReadingStreamReceiver is an unused stdin deserialization sketch, not instantiated by main.cpp.
 * Keep in mind: the shared total and broker state are part of the unvalidated module-sharing
 * experiment. Mixed diagnostic/binary stdout is illustrative, not a production transport protocol.
 * Run: companion header for the disabled Sub0Pub_CrossModule targets; see main.cpp and README.md.
 */
#pragma once
#include <iostream>

#include "sub0pub/sub0pub.hpp"


class ReadingSource : public sub0::Publish<float>
        , public sub0::Publish<int>
{
public:
    void publishPair()
    {
        const float floatData = 1.019F;
        std::cout << "ReadingSource sent float : " << floatData << std::endl;
        sub0::publish( this, floatData );

        const int intData = 2;
        std::cout << "ReadingSource sent int : " << intData << std::endl;
        sub0::publish( this, intData );
    }
};


class ReadingStreamReceiver : public sub0::StreamDeserializer<>
        , public sub0::ForwardPublish<float,ReadingStreamReceiver>
        , public sub0::ForwardPublish<int,ReadingStreamReceiver>
{
public:
    ReadingStreamReceiver()
        : sub0::StreamDeserializer<>( std::cin )
    {
    }
};


float total = 0.0F;

class ReadingAccumulator : public sub0::Subscribe<float>
    , public sub0::Subscribe<int>
{
public:
    void receive( const float& data ) noexcept
    {
        total += data;
    }
    void receive( const int& data ) noexcept
    {
        total += data;
    }
};

class ReadingStreamForwarder : public sub0::StreamSerializer<>
        , public sub0::ForwardSubscribe<float,ReadingStreamForwarder>
        , public sub0::ForwardSubscribe<int,ReadingStreamForwarder>
{
public:
    ReadingStreamForwarder() : sub0::StreamSerializer<>( std::cout )
    {}

};
