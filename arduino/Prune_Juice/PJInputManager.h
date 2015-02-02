

class PJInputManager
{
public:
	// left and right might be able to be pressed multiple times before a check
	int inputsLeft();
	int inputsRight();
	bool inputButtonHold();
	bool inputButtonClick();
	bool inputButtonDoubleClick();

protected:
	static int INPUT_BUTTON_HOLD_MIN_TIME = 100; // min amount of time to be considered button hold
	static int INPUT_BUTTON_DOUBLE_CLICK_TIME = 60; // max about of time a button can be up to be a double click

	bool m_inputsLeft = 0;
	bool m_inputsRight = 0;
	
	bool m_buttonHeld = false;
	bool m_buttonClick = false;
	bool m_buttonDoubleClick = false;

	bool m_buttonDownTime = 0;
	bool m_buttonUpTime = 0;
}