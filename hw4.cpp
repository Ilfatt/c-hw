#include <string>
#include <map>
#include <vector>
#include <cstdlib>
#include <dirent.h>

using namespace std;

int hw4(const string& directory)
{
    map<string, vector<string>> extensions;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (directory.c_str())) != nullptr) {
        while ((ent = readdir (dir)) != nullptr) {
            string filename = ent->d_name;
            string extension = filename.substr(filename.find_last_of('.') + 1);
            if (extensions.find(extension) == extensions.end()) {
                extensions[extension] = {filename};
            } else {
                extensions[extension].push_back(filename);
            }
        }
        closedir (dir);
    } else {
        perror ("");
        return EXIT_FAILURE;
    }
    for (auto &extension : extensions) {
        string folder_name = extension.first + "s";
        for (auto &filename : extension.second) {
            string src = directory + filename;
            string dst = directory + folder_name + "/" + filename;
            rename(src.c_str(), dst.c_str());
        }
    }
    return 0;
}