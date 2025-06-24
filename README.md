-----

# IKeybind: Keybind Management for Arduino

`IKeybind` is a templated C++ library for managing keybinds in Arduino projects.
It integrates with the `IPushButton` library for button input.
The library provides functionality to define and detect sequences of button presses.

-----

## Features

  * **Keybind Definition:** Supports defining keybinds as sequences of multiple buttons (`IPushButton` instances).
  * **IPushButton Integration:** Requires `IPushButton` for button state management.
  * **Sequence Prioritization:** Detects longer keybind sequences when multiple keybinds share a primary button.
  * **Modifier Key Handling:** Manages modifier buttons within a keybind to prevent conflicts with other single-button events in the same update cycle.
  * **Compile-Time Configuration:** Template parameters allow for configuration of system capacities:
      * `NKey_`: Configures the maximum number of individual buttons.
      * `NEvent_`: Configures the maximum number of distinct keybind events.
      * `KbMax_`: Configures the maximum number of buttons within a single keybind sequence.
  * **Event Status Query:** Provides methods to check the status of specific or any triggered keybind events.

-----

## Operation

`IKeybind` accepts an array of `IPushButton` objects during construction.
The `assign()` method defines keybinds by specifying a sequence of button IDs
and the required state (`IPushButton::eState`) for the primary button in that sequence.

The `update()` method, called within the main loop, performs the following actions:

1.  Updates the state of individual `IPushButton` instances.
2.  Resets internal event and modifier flags.
3.  Executes a keybind detection algorithm based on button states and push timings.

Following `update()`, the `isEvent(event_idx)` and `isAnyEvent()` methods provide information on triggered keybinds.

-----

## Dependencies

  * **IPushButton:** This library is a prerequisite. `IKeybind` utilizes `IPushButton` for button state management.

-----

## Getting Started

1.  **Install `IPushButton`:** Ensure the `IPushButton` library is available in your development environment.
2.  **Include `IKeybind`:** Add the `IKeybind.h` file to your project.
3.  **Define Buttons:** Create an `std::array` of `IPushButton` objects representing physical buttons.
4.  **Instantiate `IKeybind`:** Create an instance of the `IKeybind` template, specifying the maximum number of keys, events, and keys per keybind as template arguments.
5.  **Assign Keybinds:** Use the `assign()` method to configure key combinations and their associated events.
6.  **Update and Check:** Call `IKeybind::update()` within your `loop()` function. Use `IKeybind::isEvent()` or `IKeybind::isAnyEvent()` to determine if keybinds have been triggered.

-----

## Example Usage (Conceptual)

```cpp
#include "IKeybind.h"  // Assuming "IPushButton.h" is in your project


// --- Configuration Constants ---
const uint8_t Key_Cnt = 3;  // Number of physical buttons
const uint8_t Event_Cnt = 6;  // Number of distinct keybind events

// --- Type Aliases for Convenience ---
using MyKeybind = IKeybind<Key_Cnt, Event_Cnt>;
using eKeyState = MyKeybind::eState;

// --- Keybind Instance Initialization ---
// Instantiate MyKeybind with an array of IPushButton objects.
// The order in this array determines the internal index for each button.
// D13 (index 0), D12 (index 1), D11 (index 2)
MyKeybind kb({ {
  {D13, INPUT_PULLUP},
  {D12, INPUT_PULLUP},
  {D11, INPUT_PULLUP}
} });  // D11, D12, D13 are pin aliases defined in arduino.h


void setup()
{
	Serial.begin(115200);  // Initialize serial communication for output

	// --- Global Key Settings ---
	// Apply a repeat delay (in milliseconds) to all keys
	kb.forEachKey([](IPushButton& btn_) { btn_.repeatDelay(300); });

	// --- Assigning Keybinds to Events ---
	// The `assign` method maps a key combination and state to a unique event index.
	// Syntax: kb.assign<N-keys_in_combination>(event_index, {combination_keys}, state);
	//
	// N-keys_in_combination: The number of keys involved in this specific combination.
	// event_index: A unique integer representing the event [0, Event_Cnt).
	// combination_keys: An initializer list of the digital pins involved.
	//                   The last pin in the list is considered the "trigger" or "final" key,
	//                   while others are "modifier" keys.
	// state: The required state of the trigger key for the event to fire (e.g., release, rapid, hold).

	// This setup demonstrates binding 6 distinct events using 2 hardware buttons

	// Event 0: Triggered when D11 is released (Single key)
	kb.assign<1>(0, { D11 }, eKeyState::release);
	// Event 1: Triggered when D12 is released (Single key)
	kb.assign<1>(1, { D12 }, eKeyState::release);

	// Event 2: Triggered when D12 is pressed rapidly while D11 is held
	kb.assign<2>(2, { D12, D11 }, eKeyState::rapid);
	// Event 3: Triggered when D12 is held down while D11 is held
	kb.assign<2>(3, { D12, D11 }, eKeyState::hold);

	// Event 4: Triggered when D11 is pressed rapidly while D12 is held
	kb.assign<2>(4, { D11, D12 }, eKeyState::rapid);
	// Event 5: Triggered when D11 is held down while D12 is held
	kb.assign<2>(5, { D11, D12 }, eKeyState::hold);
}


void loop()
{
	kb.update();  // Crucial: Call update() in each loop iteration to process key states

	// Check if any defined event has been triggered
	for (int i{}; i != Event_Cnt; ++i) {
		if (kb.isEvent(i)) {
			Serial.print("Event triggered: ");
			Serial.println(i);  // Print the index of the triggered event
		}
	}

	delay(10);  // Small delay to avoid busy-waiting
}


// Possible Serial Monitor Output:
/*
	Event triggered: 0
	Event triggered: 1
	Event triggered: 2
	Event triggered: 3
	Event triggered: 4
	Event triggered: 5
*/
```

-----

## Contributing

Contributions are accepted. Please submit an issue or a pull request for proposed changes or bug fixes.

-----
