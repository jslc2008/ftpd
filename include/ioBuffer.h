#pragma once

#include <cstddef>
#include <memory>

/// \brief I/O buffer
/// [unusable][usedArea][freeArea]
class IOBuffer
{
public:
	~IOBuffer ();

	/// \brief Parameterized constructor
	/// \param size_ Buffer size
	IOBuffer (std::size_t size_);

	/// \brief Get pointer to writable area
	char *freeArea () const;
	/// \brief Get size of writable area
	std::size_t freeSize () const;

	/// \brief Get pointer to readable area
	char *usedArea () const;
	/// \brief Get size of readable area
	std::size_t usedSize () const;

	/// \brief Consume data from the beginning of usedArea
	/// \param size_ Size to consume
	/// [unusable][+++++usedArea][freeArea]
	///   becomes
	/// [unusable+++++][usedArea][freeArea]
	void markFree (std::size_t size_);
	/// \brief Produce data to the end of usedArea from freeArea
	/// [unusable][usedArea][++++++freeArea]
	///   becomes
	/// [unusable][usedArea++++++][freeArea]
	void markUsed (std::size_t size_);

	/// \brief Whether usedArea is empty
	bool empty () const;

	/// \brief Get buffer capacity
	std::size_t capacity () const;

	/// \brief Clear buffer; usedArea becomes empty
	/// [unusable][usedArea][++++++freeArea]
	///  becomes
	/// [freeArea++++++++++++++++++++++++++]
	void clear ();

	/// \brief Move usedArea to the beginning of the buffer
	/// [unusable][usedArea][freeArea]
	///  becomes
	/// [usedArea][freeArea++++++++++]
	void coalesce ();

private:
	/// \brief Buffer
	std::unique_ptr<char[]> m_buffer;

	/// \brief Buffer size
	std::size_t const m_size;

	/// \brief Start of usedArea
	std::size_t m_start = 0;
	/// \brief Start of freeArea
	std::size_t m_end = 0;
};
