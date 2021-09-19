#pragma once
#include <libOTe/config.h>
#ifdef ENABLE_SOFTSPOKEN_OT

#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Network/Channel.h>
#include "DotSemiHonest.h"

namespace osuCrypto
{
namespace SoftSpokenOT
{

// Hash DotSemiHonest to get a random OT.

class TwoOneSemiHonestSender :
	public DotSemiHonestSender,
	private ChunkedReceiver<
		TwoOneSemiHonestSender,
		std::tuple<std::array<block, 2>>,
		std::tuple<AlignedBlockPtrT<std::array<block, 2>>>
	>
{
public:
	using Base = DotSemiHonestSender;

	TwoOneSemiHonestSender(size_t fieldBits, size_t numThreads_ = 1) :
		Base(fieldBits, numThreads_),
		ChunkerBase(this) {}

	TwoOneSemiHonestSender splitBase()
	{
		throw RTE_LOC; // TODO: unimplemented.
	}

	std::unique_ptr<OtExtSender> split() override
	{
		return std::make_unique<TwoOneSemiHonestSender>(splitBase());
	}

	virtual void initTemporaryStorage() { ChunkerBase::initTemporaryStorage(); }

	void send(span<std::array<block, 2>> messages, PRNG& prng, Channel& chl) override;

	// Low level functions.

	// Perform up to 128 random OTs (assuming that the messages have been received from the
	// receiver), and output the message pairs. Set numUsed to be < 128 if you don't neeed all of
	// the messages. The number of blocks in messages (2 * messages.size()) must be at least
	// wPadded(), as there might be some padding. Also, messages must be given the alignment of an
	// AlignedBlockArray.
	void generateRandom(size_t blockIdx, size_t numUsed, span<std::array<block, 2>> messages)
	{
		block* messagesPtr = (block*) messages.data();
		Base::generateRandom(blockIdx, span<block>(messagesPtr, wPadded()));
		xorAndHashMessages(numUsed, messagesPtr);
	}

	void generateChosen(size_t blockIdx, size_t numUsed, span<std::array<block, 2>, 128> messages)
	{
		block* messagesPtr = (block*) messages.data();
		Base::generateChosen(blockIdx, span<block>(messagesPtr, wPadded()));
		xorAndHashMessages(numUsed, messagesPtr);
	}

	void xorAndHashMessages(size_t numUsed, block* messages) const;

	TRY_FORCEINLINE void processChunk(size_t numUsed, span<std::array<block, 2>> messages);

protected:
	using ChunkerBase = ChunkedReceiver<
		TwoOneSemiHonestSender,
		std::tuple<std::array<block, 2>>,
		std::tuple<AlignedBlockPtrT<std::array<block, 2>>>
	>;
	friend ChunkerBase;
	friend ChunkerBase::Base;
};

class TwoOneSemiHonestReceiver :
	public DotSemiHonestReceiver,
	private ChunkedSender<TwoOneSemiHonestReceiver, std::tuple<block>, std::tuple<AlignedBlockPtr>>
{
public:
	using Base = DotSemiHonestReceiver;

	TwoOneSemiHonestReceiver(size_t fieldBits, size_t numThreads_ = 1) :
		Base(fieldBits, numThreads_),
		ChunkerBase(this) {}

	TwoOneSemiHonestReceiver splitBase()
	{
		throw RTE_LOC; // TODO: unimplemented.
	}

	std::unique_ptr<OtExtReceiver> split() override
	{
		return std::make_unique<TwoOneSemiHonestReceiver>(splitBase());
	}

	virtual void initTemporaryStorage() { ChunkerBase::initTemporaryStorage(); }

	void receive(const BitVector& choices, span<block> messages, PRNG& prng, Channel& chl) override;

	// Low level functions.

	// Perform 128 random OTs (saving the messages up to send to the sender), and output the choice
	// bits (packed into a 128 bit block) and the chosen messages. Set numUsed to be < 128 if you
	// don't neeed all of the messages. messages.size() must be at least vPadded(), as there might
	// be some padding. Also, messages must be given the alignment of an AlignedBlockArray.
	void generateRandom(size_t blockIdx, size_t numUsed, block& choicesOut, span<block> messages)
	{
		Base::generateRandom(blockIdx, choicesOut, messages);
		mAesFixedKey.hashBlocks(messages.data(), numUsed, messages.data());
	}

	void generateChosen(size_t blockIdx, size_t numUsed, block choicesIn, span<block> messages)
	{
		Base::generateChosen(blockIdx, choicesIn, messages);
		mAesFixedKey.hashBlocks(messages.data(), numUsed, messages.data());
	}

	TRY_FORCEINLINE void processChunk(size_t numUsed, span<block> messages, block chioces);

protected:
	using ChunkerBase = ChunkedSender<
		TwoOneSemiHonestReceiver,
		std::tuple<block>,
		std::tuple<AlignedBlockPtr>
	>;
	friend ChunkerBase;
	friend ChunkerBase::Base;
};

}
}
#endif