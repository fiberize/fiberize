#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

Event<std::string> parserInput("parserInput");
Event<uint> parserOutput("parserOutput");

struct InvalidSyntax {};

struct Calculator : public Fiber<Void> {
    Calculator(FiberRef out): out(out) {}
    FiberRef out;
    
    // Lexer
    std::stringstream input;
    
    void waitForInput() {
        while (input.rdbuf()->in_avail() == 0) {
            auto moreInput = parserInput.await();
            input << moreInput;
        }
    }
    
    char getChar() {
        waitForInput();
        return input.get();
    }
    
    char peekChar() {
        waitForInput();
        return input.peek();
    }
    
    void skipSpace() {
        while (isspace(peekChar())) {
            getChar();
        }
    }
    
    uint number() {
        uint value = 0;
        if (!isdigit(peekChar())) {
            throw InvalidSyntax();
        }
        while (isdigit(peekChar())) {
            value *= 10;
            value += getChar() - '0';
        }
        return value;
    }
    
    void symbol(char c) {
        if (peekChar() == c) {
            getChar();
        } else {
            throw InvalidSyntax();
        }
    }
    
    // Parser
    
    uint atom() {
        uint value;
        
        skipSpace();
        switch (peekChar()) {
            case '(':
                symbol('(');
                value = expression();
                symbol(')');
                break;
            
            default:
                value = number();
        }
        
        return value;
    }
    
    uint expression() {
        uint value = 0;
        value += atom();
        
        skipSpace();
        if (peekChar() == '+') {
            symbol('+');            
            value += expression();
        }
        
        return value;
    }
    
    Void run() {
        while (true) {
            try {
                uint value = expression();
                skipSpace();
                symbol(';');
                out.emit(parserOutput, value);
            } catch (const InvalidSyntax&) {
                // Consume a character and try again.
                getChar();
            }
        }
    }
};

struct InputReader : public Fiber<Void> {
    InputReader(FiberRef out): out(out) {}
    FiberRef out;

    Void run() {
        std::string input;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, input);
            out.emit(parserInput, input);
        }
    }
};

int main() {
    System fiberSystem;
    FiberRef self = fiberSystem.mainFiber();
    FiberRef calc = fiberSystem.run<Calculator>(self);
    FiberRef inputReader = fiberSystem.run<InputReader>(calc);
    
    auto _printResults = parserOutput.bind([] (uint value) {
        std::cout << "= " << value << std::endl;
    });
    
    Context::current()->yield();
}