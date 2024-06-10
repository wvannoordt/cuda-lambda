main:
	$(shell which nvcc) -x cu --extended-lambda -std=c++20 main.cc -o a.out
