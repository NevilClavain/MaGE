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

#pragma once

#include <string>
#include <thread>
#include <utility> 
#include <mutex> 
#include "mailbox.h"
#include "asynctask.h"
#include "eventsource.h"

namespace mage
{
	namespace core
	{
		enum class RunnerEvent
		{
			TASK_UPDATE,
			TASK_ERROR,
			TASK_DONE
		};


		class Runner : public mage::property::EventSource<const RunnerEvent&, const std::string&, const std::string&>
		{

		public:

			Runner() = default;
			Runner(const Runner&) = delete;
			Runner(Runner&&) = delete;
			Runner& operator=(const Runner& t) = delete;

			~Runner() = default;

			struct TaskReport
			{
				RunnerEvent runner_event;
				std::string target;
				std::string action;
			};
		
			Mailbox<property::AsyncTask*>					m_mailbox_in;
			Mailbox<TaskReport>								m_mailbox_out;

			void startup(void);
			void join(void);

			void dispatchEvents();

			bool isBusy();

		private:

			void mainloop();

			mutable std::unique_ptr<std::thread>			m_thread;
			bool											m_cont;

			std::mutex										m_state_mutex;
			bool											m_busy;

			static constexpr unsigned int idle_duration_ms{ 50 };
			friend struct RunnerKiller;
		};

		struct RunnerKiller : public property::AsyncTask
		{
			RunnerKiller(void) : AsyncTask("KILL", "Runner")
			{
			}

			void execute(core::Runner* p_runner)
			{
				p_runner->m_cont = false;
			}
		};
	}
}
