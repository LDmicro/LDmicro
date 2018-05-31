/*******************************************************************************
*  file    : filetracker.hpp
*  created : 19.05.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#include <cstdio>
#include <stdexcept>


class FileTracker
{
    FileTracker(const FileTracker& ) {}
    FileTracker& operator=(const FileTracker&) {}
public:
    FileTracker(const char* name = nullptr, const char* mode = nullptr) :
        file_(nullptr)
    {
        if(name)
            open(name, mode);
    }
    ~FileTracker()
    {
        close();
    }
    bool open(const char* name, const char* mode = nullptr)
    {
        const char *mode_ = "r";
        if(mode)
            mode_ = mode;
        file_ = std::fopen(name, mode_);
        return is_open();
    }
    void close()
    {
        if(file_)
            {
                std::fclose(file_);
                file_ = nullptr;
            }
    }
    bool is_open() const
    {
        return file_ != nullptr;
    }
    FILE* get()
    {
        return file_;
    }
public:
    bool operator==(const FileTracker& other) const
    {
        return file_ == other.file_;
    }
    bool operator!=(const FileTracker& other) const
    {
        return file_ != other.file_;
    }
    operator FILE* ()
    {
        return get();
    }
    FileTracker(FileTracker&& other) :
        file_(nullptr)
    {
        file_ = other.file_;
        other.file_ = nullptr;
    }
    FileTracker& operator=(FileTracker&& other)
    {
        if(*this != other)
            {
                close();
                file_ = other.file_;
                other.file_ = nullptr;
            }
        return *this;
    }
private:
    FILE* file_;
};
