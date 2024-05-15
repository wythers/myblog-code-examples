
main: test_logger
	./test_logger

.PHONY: test_logger
test_logger: logger_test.cpp
	g++ -latomic -pthread -fconcepts -o $@ $<