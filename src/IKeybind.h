#pragma once
#include <array>
#include <stdexcept>
#include <stdint.h> // For uint8_t
#include "IPushButton.h" // Arduino-specific IPushButton class



//=== Base interface for keybinding logic ===//
class IKeybindBase
{
public:
	// Type aliases
	using self_type  = IKeybindBase;
	using size_type  = uint8_t;
	using Key        = IPushButton;
	using eState     = IPushButton::eState;

public:
	// Virtual destructor
	virtual ~IKeybindBase() = default;
	// Updates the state of all keys and checks for triggered keybind events
	virtual void update() = 0;
	// Checks if a specific keybind event has occurred
	virtual bool isEvent(uint8_t) const = 0;
	// Checks if any keybind event has occurred
	virtual bool isAnyEvent() const = 0;
};



/// @brief A templated class for managing and detecting complex keybinds.
/// This class extends IKeybindBase and provides a robust mechanism to define
/// and detect sequences of key presses (keybinds) involving multiple keys and specific states.
/// It is designed to be highly configurable through its template parameters.
///
/// @tparam NKey_ The maximum number of individual keys this keybind system can manage.
/// @tparam NEvent_ The maximum number of distinct keybind events that can be defined.
/// @tparam KbMax_ The maximum number of keys that can be part of a single keybind sequence.
template < uint8_t NKey_, uint8_t NEvent_, uint8_t KbMax_ = NKey_ >
class IKeybind : public IKeybindBase
{
public:
	// Type aliases
	using self_type  = IKeybind;
	using base_type  = IKeybindBase;
	using size_type  = uint8_t;
	using Key        = typename base_type::Key;
	using eState     = typename base_type::eState;


public:
	// Compile-time constants
	static const size_type Key_Count{ NKey_ };
	static const size_type Event_Count{ NEvent_ };
	static const size_type Keybind_Max{ KbMax_ };


private:
	/// @brief  An array holding all the individual key objects.
	std::array<Key, Key_Count> aKey;

	/// @brief A 2D array storing the key indices for each defined keybind event.
	/// `aKeybind[event_idx][key_in_sequence_idx]` stores the index of the key in `aKey`.
 	std::array<std::array<size_type, Keybind_Max>, Event_Count> aKeybind;

	/// @brief An array storing the actual number of keys in each defined keybind.
	/// `aKeybindSize[event_idx]` holds the size of the keybind at `event_idx`.
	std::array<size_type, Event_Count> aKeybindSize;

	/// @brief An array storing the required state for the primary key of each event.
	/// `aPrimaryKeyState[event_idx]` specifies the `eState` that the final key
	/// in the keybind sequence (the primary key) must be in for the event to trigger.
	std::array<eState, Event_Count> aPrimaryKeyState;

	/// @brief A boolean array indicating whether each event has occurred in the current update cycle.
	/// `aEventOccurred[event_idx]` is true if the keybind for that event was detected.
	std::array<bool, Event_Count> aEventOccurred;

	/// @brief A boolean array tracking if a key has been used as a modifier in a detected keybind.
	/// `aUsedAsModifier[key_idx]` is true if the key at `key_idx` was part of a
	/// successfully detected keybind as a modifier (not the primary key).
	std::array<bool, Key_Count> aUsedAsModifier;


private:
	// Deleted constructors
	IKeybind(const self_type&) = delete;
	IKeybind(self_type&&) = delete;

	/// @brief Checks if all keys in a given keybind sequence are in the correct state and timing.
	/// This method ensures that modifier keys are held (pushed, held, or delayed) and
	/// that their push times are in the correct sequence relative to the primary key.
	///
	/// @param event_idx_ The index of the keybind event to validate.
	/// @return True if the sequence is valid, false otherwise.
	bool isValidSequence(size_type event_idx_) const
	{
		for (size_type j{ 1 }; j < aKeybindSize[event_idx_]; ++j) {
			if (!(aKey[aKeybind[event_idx_][j]].state() & (eState::push | eState::hold | eState::delay))) { return false; }
			if (aKey[aKeybind[event_idx_][j]].pushTime() > aKey[aKeybind[event_idx_][j - 1]].pushTime()) { return false; }
		}
		return (aKey[aKeybind[event_idx_][0]].state() != eState::none);
	}

	/// @brief Checks if a specific key has been marked as a modifier in a detected keybind.
	///
	/// @param key_idx_ The index of the key to check.
	/// @return True if the key is used as a modifier, false otherwise.
	bool isUsedAsModifier(size_type key_idx_) const
	{
		return (aUsedAsModifier[key_idx_]);
	}

	/// @brief Checks if a keybind at a given event index is empty (not assigned).
	///
	/// @param event_idx_ The index of the event to check.
	/// @return True if the keybind is empty, false otherwise.
	bool isEmptyKeybind(size_type event_idx_) const
	{
		return (aKeybindSize[event_idx_] == 0);
	}

	/// @brief Marks all modifier keys within a successfully detected keybind as 'used'.
	/// This prevents these modifier keys from also being detected as primary keys
	/// for other keybinds in the same update cycle.
	///
	/// @param event_idx_ The index of the event whose modifiers should be marked.
	void markModifiersAsUsed(size_type event_idx_)
	{
		for (size_type j{ 1 }; j < aKeybindSize[event_idx_]; ++j) {
			aUsedAsModifier[aKeybind[event_idx_][j]] = true;
		}
	}

	/// @brief Core logic for searching and identifying triggered keybind events.
	/// This method iterates through all defined keybinds, validates their sequences,
	/// and marks which events have occurred. It prioritizes longer keybind sequences
	/// if multiple keybinds share the same primary key.
	void searchKeybind()
	{
		// Primary key index -> Index of the longest event using this key as primary
		std::array<size_type, Key_Count> aKeyBestEventIdx{};  // Stores (event_index +1), so 0 means "no event found yet"

		// Iterate through all defined keybinds
		for (size_type i{}; i != Event_Count; ++i) {
			// Skip if the current keybind has not been assigned (is empty)
			if (isEmptyKeybind(i)) {
				continue;
			}
			// Prefer longest sequence for primary key
			if (aKeyBestEventIdx[aKeybind[i][0]]) {
				if (aKeybindSize[i] < aKeybindSize[aKeyBestEventIdx[aKeybind[i][0]] -1]) {
					continue;
				}
				/*
					CHANGES BEGIN
				*/
				if (aKeybindSize[i] == aKeybindSize[aKeyBestEventIdx[aKeybind[i][0]] -1]) {
					if (!(aPrimaryKeyState[i] & aKey[aKeybind[i][0]].state())) {
						continue;
					}
				}
				/*
					CHANGES END
				*/

			}
			// Skip if primary key already used as modifier
			if (isUsedAsModifier(aKeybind[i][0])) {
				continue;
			}
			// Ensures all keys are in the correct state
			if (!isValidSequence(i)) {
				continue;
			}

			// If all checks pass, this keybind is a candidate
			aKeyBestEventIdx[aKeybind[i][0]] = i +1;  // store its index (plus 1)
		}

		// After evaluating all keybinds, mark the best events as occurred and their modifiers as used
		for (size_type i{}; i != Key_Count; ++i) {
			// If no event was found for this primary key, skip
			if (!aKeyBestEventIdx[i]) { continue; }
			// Check primary key state
			if (!(aPrimaryKeyState[aKeyBestEventIdx[i] -1] & aKey[i].state())) { continue; }
			// Mark the modifiers of the detected event
			markModifiersAsUsed(aKeyBestEventIdx[i] -1);
			// Mark the event as occurred
			aEventOccurred[aKeyBestEventIdx[i] -1] = true;
		}
	}


public:
	// Default destructor
	~IKeybind() = default;

	/// @brief Constructor for IKeybind.
	/// Initializes the keybind system with an array of individual key objects.
	/// All internal arrays for keybind definitions and states are default-initialized (filled with zeros/false).
	///
	/// @param keys_ An `std::array` containing all the `IPushButton` objects this keybind system will manage.
	IKeybind(std::array<Key, Key_Count> keys_) :
		aKey{ keys_ },       // Initialize the array of keys with the provided keys
		aKeybind{},          // Default-initialize the keybind definitions array
		aKeybindSize{},      // Default-initialize the keybind size array
		aPrimaryKeyState{},  // Default-initialize the primary key state array
		aEventOccurred{},    // Default-initialize the event occurrence array
		aUsedAsModifier{}    // Default-initialize the used as modifier array
	{}

	/// @brief Assigns a key sequence and a primary key state to a specific event index.
	/// This method defines what constitutes a keybind event.
	///
	/// @tparam N The number of key IDs in the `key_id_` array.
	/// @param event_idx_ The index of the event to which this keybind is being assigned.
	///                   Must be less than `Event_Count`.
	/// @param key_id_ An `std::array` of key IDs that form the keybind sequence.
	///                The last ID in this array is considered the "primary" key.
	/// @param key_state_ The required `eState` of the primary key for this keybind event to trigger.
	/// @throw std::out_of_range If `event_idx_` is out of bounds or `N` exceeds `Keybind_Max`.
	/// @throw std::invalid_argument If any `key_id_` is not found in the available keys.
	template <size_type N>
	void assign(size_type event_idx_, std::array<size_type, N> key_id_, eState key_state_)
	{
		if (event_idx_ >= Event_Count) {
			throw std::out_of_range(
				"IKeybind::assign: Event index is out of range.");
		}
		if (N > Keybind_Max) {
			throw std::out_of_range(
				"IKeybind::assign: Keybind size error.");
		}

		for (size_type i{}; i != N; ++i) {
			size_type j{};
			for (; j < Key_Count; ++j) {
				if (aKey[j].id() == key_id_[i]) {
					aKeybind[event_idx_][N - i - 1] = j;  // Convert id to idx and store in reverse order
					break;
				}
			}
			// If a key ID provided in `key_id_` was not found among the available keys
			if (j == Key_Count) {
				throw std::invalid_argument(
					"IKeybind::assign: Key ID not found in available keys.");
			}
		}
		aPrimaryKeyState[event_idx_] = key_state_;
		aKeybindSize[event_idx_] = N;
	}

	/// @brief Gets a reference to a key object by its index.
	///
	/// @param key_idx_ The index of the key to retrieve.
	/// @return A reference to the `Key` object at the specified index.
	/// @throw std::out_of_range If `key_idx_` is out of bounds.
	Key& getKey(size_type key_idx_)
	{
		if (key_idx_ >= Key_Count) {
			throw std::out_of_range(
				"IPushButton::getKey: Key index is out of range.");
		}
		return aKey[key_idx_];
	}

	/// @brief Applies a unary function to each individual key managed by this keybind system.
	///
	/// @tparam UnaryFn_ The type of the unary function (e.g., a lambda, function pointer, functor).
	/// @param fn_ The function to apply. It should take a `Key&` as an argument.
	template <typename UnaryFn_>
	void forEachKey(UnaryFn_ fn_)
	{
		for (auto& it : aKey) { fn_(it); }
	}

	/// @brief Overrides IKeybindBase::update().
	/// This method is called repeatedly to update the state of all managed keys
	/// and then to detect if any defined keybind events have occurred.
	void update() override
	{
		// Reset all event occurrence flags for the current cycle
		aEventOccurred.fill(false);
		// Update each individual key and reset its 'used as modifier' flag if it's idle or disabled
		for (size_type i{}; i != Key_Count; ++i) {
			aKey[i].update();
			if (aKey[i].state() == eState::none
				or aKey[i].state() & eState::idle) { 
				aUsedAsModifier[i] = false;
			}
		}
		// Perform the core keybind detection logic
		searchKeybind();
	}

	/// @brief Overrides IKeybindBase::isEvent().
	/// Checks if a specific keybind event has occurred in the most recent `update()` call.
	///
	/// @param event_idx_ The index of the event to check.
	/// @return True if the specified event occurred, false otherwise. Returns false if `event_idx_` is out of bounds.
	bool isEvent(size_type event_idx_) const override
	{
		if (event_idx_ >= Event_Count) { return false; }
		return aEventOccurred[event_idx_];
	}

	/// @brief Overrides IKeybindBase::isAnyEvent().
	/// Checks if any keybind event has occurred in the most recent `update()` call.
	///
	/// @return True if at least one event occurred, false otherwise.
	bool isAnyEvent() const override
	{
		for (auto& it : aEventOccurred) { if (it) { return true; } }
		return false;
	}

	/// @brief Clears all defined keybinds and resets internal state arrays.
	/// This unassigns all events and prepares the `IKeybind` object for new keybind definitions.
	void clear()
	{
		aKeybind         .fill({});
		aKeybindSize     .fill({});
		aPrimaryKeyState .fill({});
		aEventOccurred   .fill({});
		aUsedAsModifier  .fill({});
	}

};



