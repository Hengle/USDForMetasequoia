#include "pch.h"
#include "Foundation/mqusdSceneGraph.h"
#include <io.h>
#include <fcntl.h>
#include <iostream>
using namespace mqusd;

int main(int argc, char *argv[])
{
#ifdef _WIN32
    // set stdin&stdout binary mode
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    std::string usd_path;
    std::string file_path;
    double time = std::numeric_limits<double>::quiet_NaN();
    bool out = false;
    for (int ai = 1; ai < argc;) {
        if (argv[ai][0] == '-') {
            if (strcmp(argv[ai], "-o") == 0)
                out = true;
            if (strcmp(argv[ai], "-f") == 0)
                file_path = argv[++ai];
            if (strcmp(argv[ai], "-t") == 0)
                sscanf(argv[++ai], "%lf", &time);
        }
        else
            usd_path = argv[ai];
        ++ai;
    }

    if (usd_path.empty()) {
        printf("usage: %s (options) path_to_usd.usd\n"
            "options:\n"
            "-t time_in_seconds\n"
            "-o\n"
            "-of path_to_output_file\n"
            , argv[0]);
        return 0;
    }

    mqusd::SetModuleDir(mu::GetCurrentModuleDirectory());
    auto scene = mqusd::CreateUSDScene();
    if (!scene)
        return 1;

    if (out) {
        try {
            if (!scene->create(usd_path.c_str()))
                return 1;

            if (!file_path.empty()) {
                std::fstream f(file_path.c_str(), std::ios::in | std::ios::binary);
                scene->deserialize(f);
            }
            else {
                scene->deserialize(std::cin);
            }
            scene->write(time);
            scene->save();
        }
        catch (std::exception& e) {
            printf("error: %s\n", e.what());
        }
    }
    else {
        if (!scene->open(usd_path.c_str()))
            return 1;

        scene->read(time);
        if (!file_path.empty()) {
            std::fstream of(file_path.c_str(), std::ios::out | std::ios::binary);
            scene->serialize(of);
        }
        else {
            scene->serialize(std::cout);
        }
    }
    return 0;
}
