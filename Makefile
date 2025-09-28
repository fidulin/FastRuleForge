#generic makefile for fastruleforge
CXX = clang++
CXXFLAGS = -std=c++17 -MMD -MP -fmodules -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include
CPPFLAGS = -I./libs/metal-cpp
OBJCPPFLAGS = -fobjc-arc
LDFLAGS = -framework Foundation -framework Metal -framework QuartzCore -L/opt/homebrew/opt/libomp/lib
LDLIBS = -lomp

SRCS_CC = src/main.cc src/GPU_executor.cc src/rule_generator.cc src/args_handler.cc src/utils.cc
SRCS_MM = src/metal_cpp.mm
SRCS = $(SRCS_CC) $(SRCS_MM)
OBJS = $(SRCS_CC:src/%.cc=build/%.o) $(SRCS_MM:src/%.mm=build/%.o)
DEPS = $(OBJS:.o=.d)

TARGET = fastruleforge

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LDLIBS)

build/%.o: src/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

build/%.o: src/%.mm
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OBJCPPFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(TARGET) build
