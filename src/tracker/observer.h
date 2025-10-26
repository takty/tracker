/**
 * Observer
 *
 * @author Takuto Yanagida
 * @version 2020-03-22
 */

#pragma once

class Observer {

public:

	virtual void Updated() = 0;

	Observer() noexcept = default;

	Observer(const Observer&) = delete;
	virtual Observer& operator=(const Observer&) = delete;
	Observer(Observer&&) = delete;
	virtual Observer& operator=(Observer&&) = delete;

	~Observer() = default;

};
