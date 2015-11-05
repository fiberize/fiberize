#include <fiberize/fiberize.hpp>
#include <iostream>

using namespace fiberize;

struct InvalidSyntax {};

struct Calculator : public Fiber<Void> {
    static Event<std::string> feed;
    static Event<uint> result;
    static Event<FiberRef> subscribe;

    // Lexer
    std::stringstream input;
    
    void waitForInput() {
        while (input.rdbuf()->in_avail() == 0) {
            auto moreInput = feed.await();
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
    
    // Driver
    
    std::vector<FiberRef> subscribers;
    
    Void run() {
        auto _handleSubscription = subscribe.bind([this] (const FiberRef& fiber) {
            subscribers.push_back(fiber);
        });
        
        while (true) {
            try {
                uint value = expression();
                skipSpace();
                symbol(';');
                
                process();
                for (auto fiber : subscribers) {
                    fiber.emit(result, value);
                }
            } catch (const InvalidSyntax&) {
                // Consume a character and try again.
                getChar();
            }
        }
    }
};

Event<std::string> Calculator::feed("Calculator::feed");
Event<uint> Calculator::result("Calculator::result");
Event<FiberRef> Calculator::subscribe("Calculator::subscribe");

int main() {
    System fiberSystem;
    
    FiberRef calc = fiberSystem.run<Calculator>();
    calc.emit(Calculator::subscribe, fiberSystem.currentFiber());
    
    auto _printResults = Calculator::result.bind([] (uint value) {
        std::cout << value << std::endl;
    });

    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        calc.emit(Calculator::feed, line);
        Context::current()->process();
    }
}
