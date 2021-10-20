#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <iostream>
#include <future>
#include <poll.h>
#include <signal.h>
#include <optional>
#include <chrono>
#include <mutex>
#include <sys/stat.h>

#include "lab1_cpp/trialfuncs.hpp"
#include "pickle_op_result.hpp"
 
using namespace os::lab1::compfuncs;
 
#define SELECTED_OP INT_SUM
#define TRIAL_RESULT_TYPE op_group_traits<SELECTED_OP>::result_type

std::mutex file_accces_m; 
template<typename op_result_type> 
void trial_wrapper(op_result_type (*trial)(int), int case_nr,const char* pipe_name) {
	op_result_type res;
	for(uint i = 0; i < 10; i++)
	{
		res = trial(case_nr);
		if (res.index() != 1) // Soft fail
			break;
	}
	std::string res_dumped = result_pickle::dumps(res);
	std::cout << res_dumped << std::endl;
	int fd;
	char buf[32];
	file_accces_m.lock();
	fd = open(pipe_name, O_RDWR);
	write(fd, buf, snprintf(buf, 32, "%s\n", res_dumped.c_str()));
	close(fd);
	file_accces_m.unlock();
}

bool ctrl_c_pressed = 0;
void ctrl_c_handler(int s){
	if (ctrl_c_pressed)
		exit(1);
	if (s == 2) {
		ctrl_c_pressed = true;
	}
}

template<typename op_result_type>
int read_return_value(const char* pipe_name, op_result_type &res){
	int fd;
	struct pollfd pfd;
	pfd.events = POLLIN;
	char buf[PIPE_BUF];
	fd = pfd.fd = open(pipe_name, O_RDONLY|O_NONBLOCK);
	fcntl(fd, F_SETFL, 0);
	int suc = poll(&pfd, 1, 100);
	if (suc)
		read(fd, buf, PIPE_BUF);
	close(fd);
	if (!suc)
		return 0;
	std::string result_str(buf);
	res = result_pickle::loads(result_str);
	return 1;
}

static std::string getAnswer()
{    
    std::string answer;
    std::cin >> answer;
    return answer;
}

int main()
{	
	char pipe_name_f[] = "out_f";
	char pipe_name_g[] = "out_g";
	mkfifo(pipe_name_f, 0666);
	mkfifo(pipe_name_g, 0666);

	int task_number;
	std::cout << "Choose task 0 or 1\n";
	std::cin >> task_number;


	auto f = std::async(std::launch::async, trial_wrapper<TRIAL_RESULT_TYPE>, trial_f<SELECTED_OP>, task_number, pipe_name_f);
	auto g = std::async(std::launch::async, trial_wrapper<TRIAL_RESULT_TYPE>, trial_g<SELECTED_OP>, task_number, pipe_name_g);
	std::optional<TRIAL_RESULT_TYPE> trial_g_result;
	std::optional<TRIAL_RESULT_TYPE> trial_f_result;
	int i = 0;
	// signal(SIGINT, ctrl_c_handler);
	bool prompt_to_cancell_avaliable = true;
	std::future<std::string> user_input_future;
	std::chrono::system_clock::time_point last_cancellation_prompt_time = std::chrono::system_clock::now();
	for (;;i++) {
		if (std::chrono::system_clock::now() - last_cancellation_prompt_time > std::chrono::seconds(5) && prompt_to_cancell_avaliable){
			std::cout << "Stop? 1. [C] - Continue 2. [N] - Continue dont show prompt 3. [S] - Stop\n";
			std::string input;
			std::cin >> input;
			last_cancellation_prompt_time = std::chrono::system_clock::now();
			if (input[0] == 'N')
			{
				prompt_to_cancell_avaliable = false;
			}
			else if (input[0] == 'S')
			{
				break;
			}
			else if (input[0] != 'C')
			{
				last_cancellation_prompt_time -= std::chrono::seconds(5);
			}
		}
		if (!trial_f_result){
			// std::cout << "Got f:"  << std::endl;
			read_return_value(pipe_name_f, trial_f_result);
		}
		if (!trial_g_result){
			// std::cout << "Got g:"  << std::endl;
			read_return_value(pipe_name_g, trial_g_result);
		}
		if (trial_f_result and trial_g_result)
			break;
	}

	if (trial_g_result)
		std::cout << "G:" << trial_g_result.value() << std::endl;
	else 
		std::cout << "G is not computed" << std::endl;
	if (trial_f_result)
		std::cout << "F:" << trial_f_result.value() << std::endl;
	else 
		std::cout << "F is not computed" << std::endl;
	if (trial_g_result && trial_f_result && trial_g_result.value().index() == 2 && trial_f_result.value().index() == 2)
		std::cout << "Result: " << (std::get<2>(trial_g_result.value()) + std::get<2>(trial_f_result.value())) << std::endl;

	exit(0);
}
