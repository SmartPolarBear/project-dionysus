/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-13 23:05:56
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-09-13 23:09:06
 * @ Description: A tool to make disk image for testing
 */

#include "iostream"
#include "string"
#include "filesystem"

using std::cin;
using std::cout;
using std::endl;
using std::string;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: diskimg target stage1 stage2 [distdir]" << std::endl;
    }

    const auto cur_path = string{argv[0]};
    const auto target = string{argv[1]};

    return 0;
}
