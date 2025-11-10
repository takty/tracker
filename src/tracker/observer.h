/**
 * Observer
 *
 * @author Takuto Yanagida
 * @version 2025-11-10
 */

#pragma once

class Observer {

public:

	virtual void updated() = 0;

	Observer() noexcept = default;
	Observer(const Observer&) = delete;
	virtual Observer& operator=(const Observer&) = delete;
	Observer(Observer&&) = delete;
	virtual Observer& operator=(Observer&&) = delete;
	~Observer() = default;

};
