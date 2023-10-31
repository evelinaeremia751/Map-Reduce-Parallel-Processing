#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <vector>
#include <math.h>
#include <set>
#include <unordered_set>

using namespace std;
//structura pentru a transmite parametrii
struct argument_parametru {
    pthread_mutex_t *mutex;
    pthread_barrier_t *barrier;
    vector<vector<set<int>>> *val_to_process;
    int id;
    vector<string> *texts;

    int number_of_threads_mapper, number_of_threads_reducer;
    int global_thread_ID;
};

bool power_aux(long long int nr, long long int exponent, long long int start, long long int end) {
    if (start > end)
        return false;

    long long int mid = (start + end) / 2;

    long long int power = pow(mid, exponent);

    if (power == nr)
        return true;

    if (nr > power)
        return power_aux(nr, exponent, mid + 1, end);
    else 
        return power_aux(nr, exponent, start, mid - 1);
        
}

bool check_if_power(int nr, int exponent)
{
    if (nr == 1)
        return true;
    else if (nr == 0)
        return false;
    return power_aux(nr, exponent, 2, sqrt(nr) + 1);
}


void *f(void *args) {
    argument_parametru date = *((argument_parametru *) args);
    bool all_documents_processed = false;

    int id = date.id;
    pthread_barrier_t *barrier = date.barrier;
    pthread_mutex_t *mutex = date.mutex;
    vector<vector<set<int>>> *val_to_process = date.val_to_process;
    vector<string> *texts = date.texts;
    int number_of_threads_mapper = date.number_of_threads_mapper;
    int number_of_threads_reducer = date.number_of_threads_reducer;
    int thread_global_ID = date.global_thread_ID;

    string file;

    bool found_document = false;

    // Mapper
    if (thread_global_ID < number_of_threads_mapper) {
        while (!all_documents_processed)
        {
            found_document = false;
            pthread_mutex_lock(mutex);
            for (unsigned int i = 0; i < texts->size(); i++)
            {
                if (texts->at(i) != "")
                {
                    file = texts->at(i);
                    texts->at(i) = "";
                    found_document = true;
                    break;
                }
            }

            if (!found_document)
                all_documents_processed = true;

            pthread_mutex_unlock(mutex);
            if (!all_documents_processed)
            {
                // aici procesez un fisier al carui nume se afla in file
                ifstream my_file;
                my_file.open(file);

                int n;

                my_file >> n;
                for (int i = 0; i < n; i++)
                {
                    int nr;
                    my_file >> nr;
                    for (int j = 2; j <= number_of_threads_reducer + 1; j++)
                    {
                        if (check_if_power(nr, j))
                        {
                            val_to_process->at(id).at(j - 2).insert(nr);
                        }
                    }
                }
            }
        }

    }

    pthread_barrier_wait(barrier);

    // Reducer
    if (thread_global_ID >= number_of_threads_mapper) {
        set<int> set_final;

        for (int i = 0; i < number_of_threads_mapper; i++) {
            set<int>::iterator it1 = val_to_process->at(i).at(id).begin();
            set<int>::iterator it2 = val_to_process->at(i).at(id).end();

            set_final.insert(it1, it2);
        }

        ofstream fisier_output;
        fisier_output.open("out" + to_string(id + 2) + ".txt");
        fisier_output << set_final.size();
    }

}

int main(int argc, char **argv)
{
    vector<string> texts;
    pthread_mutex_t mutex;
    pthread_barrier_t barrier;
    int number_of_threads_mapper, number_of_threads_reducer, nrTexts;

    ifstream my_file;
    number_of_threads_mapper = atoi(argv[1]);
    number_of_threads_reducer = atoi(argv[2]);


    my_file.open(argv[3]);
    my_file >> nrTexts;
    texts.reserve(nrTexts);

    for (int i = 0; i < nrTexts; i++)
    {
        string text;
        my_file >> text;
        texts.push_back(text);

        cout << text << endl;
    }

    vector<vector<set<int>>> val_to_process;

    for (int i = 0; i < number_of_threads_mapper; i++) {
        val_to_process.push_back(vector<set<int>>());

        for (int j = 0; j < number_of_threads_reducer; j++) {
            val_to_process.at(i).push_back(set<int>());
        }
    }

    pthread_t threads[number_of_threads_mapper + number_of_threads_reducer];
    argument_parametru arg_parametru[number_of_threads_mapper + number_of_threads_reducer];
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, number_of_threads_mapper + number_of_threads_reducer);

    for (int i = 0; i < number_of_threads_mapper+number_of_threads_reducer; i++) {
        arg_parametru[i].global_thread_ID = i;

        if (i < number_of_threads_mapper) {
            arg_parametru[i].id = i;
        }
        else {
            arg_parametru[i].id = i - number_of_threads_mapper;
        }
        arg_parametru[i].val_to_process = &val_to_process;
        arg_parametru[i].texts = &texts;
        arg_parametru[i].mutex = &mutex;
        arg_parametru[i].number_of_threads_mapper = number_of_threads_mapper;
        arg_parametru[i].number_of_threads_reducer = number_of_threads_reducer;
        arg_parametru[i].barrier = &barrier;
    }

    for (int i = 0; i < number_of_threads_mapper + number_of_threads_reducer; i++)
    {
        pthread_create(&threads[i], NULL, f, &arg_parametru[i]);
    }

    for (int i = 0; i < number_of_threads_mapper + number_of_threads_reducer; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);

}