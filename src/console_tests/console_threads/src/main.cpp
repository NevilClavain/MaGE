
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
#include <vector>

#include "runner.h"
#include "filesystem.h"


class Loader : public mage::property::AsyncTask
{
public:
	Loader(const std::string& p_path, std::string& p_dest): AsyncTask("loading text", "std::string"),
	m_path(p_path),
	m_dest(p_dest)
	{
	}

private:
	std::string m_path;
	std::string& m_dest;

	void execute(mage::core::Runner* p_runner)
	{
		mage::core::FileContent<const char> reader(m_path);
		reader.load();

		const std::string textIn(reader.getData(), reader.getDataSize());

		m_dest = textIn;
	}

};


int main( int argc, char* argv[] )
{    
	std::cout << "Threads tests... !\n";

	std::string text1;
	std::string text2;

	mage::core::RunnerKiller runnerKiller;

	class NotCopyConstructible
	{
	public:
		NotCopyConstructible() = default;

		std::unique_ptr<int[]> array;		
	};

	std::cout << std::is_copy_constructible<int>::value << "\n";
	std::cout << std::is_copy_constructible<NotCopyConstructible>::value << "\n";
	std::cout << std::is_copy_constructible<mage::core::SimpleAsyncTask<>>::value << "\n";
	std::cout << std::is_copy_constructible<mage::core::RunnerKiller>::value << "\n";
	

	
	mage::core::SimpleAsyncTask<const std::string&> it( "Say hello from path", "stdout",
		[&text1](const std::string& p_path)
		{
			std::cout << "Hello from path : " << p_path << "\n";
		},

		"./console_threads_assets/gmreadme.txt"
	);
	
	mage::core::SimpleAsyncTask<> it2("Say hello from nobody", "stdout",

		[](void)
		{
			std::cout << "Hello from NOBODY !\n";

		}
	);


	Loader loader("./console_threads_assets/gmreadme.txt", text1);

	Loader loader2("./console_threads_assets/title.txt", text2);


	mage::core::Runner runner;

	/*
	runner.m_mailbox_in.push<mage::property::AsyncTask*>(&it);	
	runner.m_mailbox_in.push<mage::property::AsyncTask*>(&it2);
	runner.m_mailbox_in.push<mage::property::AsyncTask*>(&loader);	
	runner.m_mailbox_in.push<mage::property::AsyncTask*>(&runnerKiller);

	mage::core::Runner runner2;
	runner2.m_mailbox_in.push<mage::property::AsyncTask*>(&loader2);
	runner2.m_mailbox_in.push<mage::property::AsyncTask*>(&runnerKiller);
	*/

	runner.m_mailbox_in.push(&it);
	runner.m_mailbox_in.push(&it2);
	runner.m_mailbox_in.push(&loader);
	runner.m_mailbox_in.push(&runnerKiller);

	mage::core::Runner runner2;
	runner2.m_mailbox_in.push(&loader2);
	runner2.m_mailbox_in.push(&runnerKiller);

	
	const auto runnerEventHandler{
		[](mage::core::RunnerEvent p_event, const std::string& p_target_descr, const std::string& p_action_descr)
		{
			if (mage::core::RunnerEvent::TASK_DONE == p_event)
			{
				std::cout << "TASK_DONE " << p_target_descr << " " << p_action_descr << "\n";
			}
		}
	};
	
	runner.registerSubscriber(runnerEventHandler);
	runner2.registerSubscriber(runnerEventHandler);


	runner.startup();
	runner2.startup();

	std::cout << ">>>>>>>runner.join\n";
	runner.join();

	std::cout << ">>>>>>>runner2.join\n";
	runner2.join();

	std::cout << "ALL joined\n";
	
	std::cout << "*****text1***************\n";
	std::cout << text1 << "\n";

	std::cout << "*****text2***************\n";
	std::cout << text2 << "\n";
	

	std::cout << "bye...\n";

    return 0;
}
