#ifndef UTILS_H
#define UTILS_H

#include <opencv2/core.hpp>

#include "dirent.h"
#include <iostream>

vector<string> list_folder(string path)
{
    vector<string> folders;
    DIR *dir = opendir(path.c_str());
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
        {
            string folder_path = path + "/" + string(entry->d_name);
            folders.push_back(folder_path);
        }
    }
    closedir(dir);

    return folders;
}

vector<string> list_file(string folder_path)
{
    vector<string> files;
    DIR *dir = opendir(folder_path.c_str());
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
        {
            string file_path = folder_path + "/" + string(entry->d_name);
            // cout << file_path << endl;
            files.push_back(file_path);
        }
    }
    closedir(dir);

    return files;
}

#endif