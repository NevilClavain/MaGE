/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2025
*
* This file is part of MaGE.
*
*    MaGE is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    MaGE is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with MaGE.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/* -*-LIC_END-*- */

#include <iostream>

#include <string>
#include <chrono>

#include "runner.h"

using namespace mage;
using namespace mage::core;
using namespace mage::property;

void Runner::mainloop()
{
	m_cont = true;

	do
	{
		auto mb_in{ &m_mailbox_in };
		auto mb_out{ &m_mailbox_out };
		const auto mbsize{ mb_in->getBoxSize() };

		if (mbsize > 0)
		{
			AsyncTask* current{ nullptr };
			do
			{
				current = mb_in->popNext(nullptr);
				if (current)
				{	
					auto task_target{ current->getTargetDescr() };
					auto task_action{ current->getActionDescr() };

					m_state_mutex.lock();
					m_busy = true;
					m_state_mutex.unlock();

					current->execute(this);

					const TaskReport report{ RunnerEvent::TASK_DONE, task_target, task_action };
					mb_out->push(report);

					m_state_mutex.lock();
					m_busy = false;
					m_state_mutex.unlock();
				}

			} while (current);
		}
		else
		{
			// idle for a little time...
			std::this_thread::sleep_for(std::chrono::milliseconds(idle_duration_ms));
		}

	} while (m_cont);	
}

bool Runner::isBusy()
{
	bool state_copy;

	m_state_mutex.lock();
	state_copy = m_busy;
	m_state_mutex.unlock();

	return state_copy;
}

void Runner::startup(void)
{	
	m_thread = std::make_unique<std::thread>(&Runner::mainloop, this);
};

void Runner::join(void)
{
	if (m_thread.get())
	{
		m_thread->join();
		dispatchEvents(); // final dispatch events
	}	
}

void Runner::dispatchEvents()
{
	const auto mb_size{ m_mailbox_out.getBoxSize() };
	for (int i = 0; i < mb_size; i++)
	{	
		auto task_report{ m_mailbox_out.popNext(TaskReport()) };
		for (const auto& call : m_callbacks)
		{
			call(task_report.runner_event, task_report.target, task_report.action);
		}
	}
}
