#include <JsonPlus/JsonPlus.hpp>
#include <iostream>

int main()
{

    auto config = JsonPlus::Load("./example1.json");

    if (config.has_value())
    {
        auto str = config.value().dump(4);

        std::cout << str << std::endl;
    }
    else
    {
        std::cerr << config.error() << std::endl;
        return 1;
    }
    return 0;
}
