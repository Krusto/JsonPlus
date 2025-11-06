#include <JsonPlus/JsonPlus.hpp>

int main()
{

    auto config = JsonPlus::Load("./example1.json");

    if (std::holds_alternative<std::string>(config))
    {
        std::cout << std::get<std::string>(config) << std::endl;
        return 0;
    }
    else
    {
        auto str = std::get<nlohmann::json>(config).dump(4);

        std::cout << str << std::endl;
    }
    return 0;
}
