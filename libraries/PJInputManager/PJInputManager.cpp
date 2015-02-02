#include "Arduino.h";
#include "PJInputManager.h";


PJInputManager::PJInputManager(int pinLeft, int pinRight, int pinButton)
{
	m_pinLeft = pinLeft;
	m_pinRight = pinRight;
	m_pinButton = pinButton;

	pinMode(m_pinLeft,INPUT_PULLUP);
	pinMode(m_pinRight,INPUT_PULLUP);
	pinMode(m_pinButton,INPUT_PULLUP);

	resetInputs();
	m_buttonDownTime = -1;
	m_buttonUpTime = -1;
	m_isDownLeft = false;
	m_isDownRight = false;
	///digitalRead(m_pinSOMETHING)  output HIGH == unpressed
}

void PJInputManager::resetInputs()
{
	m_inputDirection = 0;
	m_buttonHold = false;
	m_buttonClick = false;
	m_buttonDoubleClick = false;

}

void PJInputManager::update()
{
	//default outward facing values

	resetInputs();

	if(digitalRead(m_pinLeft) == LOW && m_isDownLeft == false)
	{
		m_inputDirection = -1;
		m_isDownLeft = true;
	}
	else if( digitalRead(m_pinLeft) == HIGH && m_isDownLeft == true)
	{
		m_isDownLeft = false;
	}
	
	
	if(digitalRead(m_pinRight) == LOW && m_isDownRight == false)
	{
		m_inputDirection = 1;
		m_isDownRight = true;
	}
	else if( digitalRead(m_pinRight) == HIGH && m_isDownRight == true)
	{
		m_isDownRight = false;
	}


	// do stuff to check for updates here
}