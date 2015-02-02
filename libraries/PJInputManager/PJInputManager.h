#ifndef PJInputManager_H
#define PJInputManager_H

#include "Arduino.h";

class PJInputManager
{
	public:
		PJInputManager(int pinLeft, int pinRight, int pinButton);

		int inputDirection(){return m_inputDirection;};
		bool inputButtonHold(){return m_buttonHold;};
		bool inputButtonClick(){return m_buttonClick;};
		bool inputButtonDoubleClick(){return m_buttonDoubleClick;};

		void update();

	private:
		//const static int INPUT_BUTTON_HOLD_MIN_TIME = 1000; // min amount of time to be considered button hold
		//const static int INPUT_BUTTON_DOUBLE_CLICK_TIME = 250; // max about of time a button can be up to be a double click

		int m_pinLeft;
		int m_pinRight;
		int m_pinButton;

		int m_inputDirection; // 1 = right, -1 = lefft, 0 = nothing
		bool m_isDownLeft;
		bool m_isDownRight;

		bool m_buttonHold;
		bool m_buttonClick;
		bool m_buttonDoubleClick;

		int m_buttonDownTime;
		int m_buttonUpTime;

		void resetInputs();
};
#endif // PJInputManager_H