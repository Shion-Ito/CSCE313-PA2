#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <errno.h>
#include <unistd.h>
#include <ctime>
#include <vector>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>

using namespace std;

string trim(string line)
{
    bool check = true;
    while (check)
    {
        if (line[line.size() - 1] == ' ')
        {
            line = line.substr(0, line.size() - 1);
        }
        if (line[0] == ' ')
        {
            line = line.substr(1);
        }
        else
            check = false;
    }

    return line;
}

char **vec_to_char_array(vector<string> &parts)
{
    char **result = new char *[parts.size() + 1]; // add 1 for the NULL
    for (int i = 0; i < parts.size(); i++)
    {
        result[i] = (char *)parts[i].c_str();
    }
    result[parts.size()] = NULL;
    return result;
}

vector<string> split(string line, string seperator)
{
    vector<string> results;
    vector<int> check;
    bool barEvent = false;
    if (seperator == "\"" || seperator == "'")
    {
        results.push_back("echo");
    }
    else if ((seperator == "<" || seperator == ">" || seperator == "|") && line.find("echo") != string::npos)
        barEvent = true;

    int next = 0;
    size_t start = 0;
    size_t found = line.find(seperator);

    int once = 0;

    if (found == 0)
    {
        once = -1;
        next = 1;
    }
    while (line.find(seperator, found + next) != string::npos)
    {
        found = line.find(seperator, found + next);

        if (barEvent)
            break;

        string sub = line.substr(start + next, found - start + once);
        if (sub == "" || sub == seperator)
        {
            continue;
        }
        else
        {
            if (sub[0] == ' ')
                sub = sub.substr(1, sub.size() - 1);
            results.push_back(sub);
        }
        start = found++;
        once = -1;
        next = 1;
    }

    string sub = line.substr(start + next, line.size() - start + next);
    if (sub[0] == ' ')
        sub = sub.substr(1, sub.size() - 1);
    results.push_back(sub);

    return results;
}

int main()
{
    char buf[100];

    vector<int> bgs;
    dup2(0, 3);
    string previous_dir = getcwd(buf, sizeof(buf));

    while (true)
    {
        //cout << previous_dir << endl;

        for (int i = 0; i < bgs.size(); i++)
        {
            if (waitpid(bgs[i], 0, WNOHANG) == bgs[i])
            {
                cout << "Zombie Process " << bgs[i] << " reaped" << endl;
                bgs.erase(bgs.begin() + i);
                i--;
            }
        }

        time_t tt;
        struct tm *ti;
        time(&tt);
        ti = localtime(&tt);

        string time = asctime(ti);
        time = time.substr(0, time.size() - 1);
        //system("Color E4");
        string user = getenv("USER");
        cout << user << "@ (" << time << "):$ "; //print a prompt
        string inputline;
        dup2(3, 0);
        //system("Color 07");

        getline(cin, inputline); //get a line from standard input
        if (inputline == string("exit"))
        {
            cout << "Bye!! End of shell" << endl;
            break;
        }

        // inputline = "ls | grep a"
        bool line = false;

        if (inputline.find("|") != string::npos)
            line = true;
        vector<string> pparts = split(inputline, "|");
        for (int i = 0; i < pparts.size(); i++)
        {
            int fds[2];
            pipe(fds);
            if (line)
            {
                inputline = pparts.at(i);
                inputline = trim(inputline);
            }

            bool bg = false;
            inputline = trim(inputline);

            if (inputline[inputline.size() - 1] == '&')
            {
                bg = true;
                inputline = inputline.substr(0, inputline.size() - 2);
            }

            if (trim(inputline).find("cd") != string::npos)
            {

                string dirname = trim(split(inputline, " ")[1]);
                if (dirname == "-")
                {
                    string temp = getcwd(buf, sizeof(buf));
                    chdir(previous_dir.c_str());
                    previous_dir = temp;
                }
                else
                {
                    previous_dir = getcwd(buf, sizeof(buf));
                    chdir(dirname.c_str());
                }

                continue;
            }

            if (trim(inputline).find("pwd") != string::npos)
            {

                char buf[1000];
                getcwd(buf, sizeof(buf));
                cout << buf << endl;
                continue;
            }

            int pid = fork();
            if (pid == 0)
            { // child process

                size_t foundEcho = inputline.find("echo");

                if (foundEcho != string::npos)
                {
                    //cout << " You are here" <<endl;
                    size_t singleQuote = inputline.find("'");
                    if (singleQuote != string::npos)
                    {
                        string Echo = inputline.substr(singleQuote, inputline.size());
                        Echo = trim(Echo);
                        if (i < pparts.size() - 1)
                        {
                            dup2(fds[1], 1);
                            close(fds[1]);
                        }
                        vector<string> parts = split(Echo, "'");
                        char **args = vec_to_char_array(parts);
                        execvp(args[0], args);
                    }
                    else
                    {
                        size_t quote = inputline.find("\"");
                        string Echo = inputline.substr(quote, inputline.size());

                        if (i < pparts.size() - 1)
                        {
                            dup2(fds[1], 1);
                            close(fds[1]);
                        }
                        Echo = trim(Echo);
                        //cout << "Where des my journey end? -> " <<Echo<< endl;
                        vector<string> parts = split(Echo, "\"");
                        //cout << parts[0] << "a" <<endl;
                        char **args = vec_to_char_array(parts);
                        execvp(args[0], args);
                    }
                }
                else
                {
                    //inputline = "ls > a.txt"
                    //looking for '>'
                    int pos = inputline.find('>');
                    if (pos >= 0)
                    {
                        inputline = trim(inputline);
                        string command = inputline.substr(0, pos);
                        string filename = inputline.substr(pos + 1);

                        inputline = trim(command);
                        filename = trim(filename);
                        int fd = open(filename.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                        dup2(fd, 1);
                        close(fd);
                    }
                    //looking for '<'
                    pos = inputline.find('<');
                    if (pos >= 0)
                    {
                        inputline = trim(inputline);
                        string command = inputline.substr(0, pos);
                        string filename = inputline.substr(pos + 1);

                        inputline = trim(command);
                        filename = trim(filename);

                        int fd = open(filename.c_str(), O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
                        dup2(fd, 0);
                        close(fd);
                    }
                
                size_t foundAwk = inputline.find("awk");

                if (foundAwk != string::npos)
                {
                    //cout << " You are here" <<endl;
                    size_t singleQuote = inputline.find("'");
                    if (singleQuote != string::npos)
                    {
                        string Echo = inputline.substr(singleQuote, inputline.size());
                        Echo = trim(Echo);
                        if (i < pparts.size() - 1)
                        {
                            dup2(fds[1], 1);
                            close(fds[1]);
                        }
                        vector<string> parts = split(Echo, "'");
                        if (foundAwk != string::npos)
                            parts[0] = "awk";
                        char **args = vec_to_char_array(parts);
                        execvp(args[0], args);
                    }
                }
                    if (i < pparts.size() - 1)
                    {
                        dup2(fds[1], 1);
                        close(fds[1]);
                    }
                
                    //cout << inputline << "!" << endl;
                    vector<string> parts = split(inputline, " ");

                    char **args = vec_to_char_array(parts);
                    execvp(args[0], args);
                
            }
            }
            else
            {
                if (!bg)
                {
                    if (i == pparts.size() - 1)
                    {
                        waitpid(pid, 0, 0); // parent waits on child process to end
                    }
                    else
                    {
                        bgs.push_back(pid);
                    }
                }
                else
                {
                    bgs.push_back(pid);
                }
                dup2(fds[0], 0);
                close(fds[1]);
            }
        }
    }
}

// https://www.geeksforgeeks.org/stringstream-c-applications/

// use string find to find where index of where '|' occurs
// use string.begin and string.end to store into a vector string pointer
// make sure to increment pointer when going to next one, then while loop until pointer reaches null and loop to execute excvp