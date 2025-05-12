#generic makefile for fastruleforge
CXX = g++
CXXFLAGS = -fopenmp -std=c++14 -MMD -MP
LDFLAGS = -lOpenCL

SRCS = src/main.cc src/GPU_executor.cc src/rule_generator.cc src/args_handler.cc src/utils.cc
OBJS = $(SRCS:src/%.cc=build/%.o)
DEPS = $(OBJS:.o=.d)

TARGET = fastruleforge

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

build/%.o: src/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)
