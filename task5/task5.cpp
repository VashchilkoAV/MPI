#include <mpi.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#define MAX_TOKEN 1000*1000*1000

typedef std::vector<size_t> Number;


Number convertToNumber(const std::string& line) {
    Number number;

    for (int i = static_cast<int>(line.length()); i > 0; i-= 9) {
        if (i >= 9) {
            number.push_back(std::stoul(line.substr(i - 9, 9)));
        } else {
            number.push_back(std::stoul(line.substr(0, i)));
        }
    }
    while ((number.size() > 1) && number.back() == 0) {
        number.pop_back();
    }

    return number;
}


void alignToCommonSize(Number* number, const size_t N) {
    while (number->size() < N) {
        number->push_back(0);
    }
    return;
}


int summarize(Number* answer, const Number& first, const Number& second, size_t left, size_t right, size_t overflow) {
    for (size_t i = 0; i < right - left; ++i) {
        answer->data()[i] = first[left + i] + second[left + i] + overflow;
        if (answer->data()[i] >= MAX_TOKEN) {
            overflow = 1;
            answer->data()[i] -= MAX_TOKEN;
        } else {
            overflow = 0;
        }
    }

    return static_cast<int>(overflow);
}


size_t countDigits(size_t n) {
    size_t digits = 1;
    unsigned long k = 10;
    for (int i = 0; i < 10; i++) {
        if (n >= k) {
            digits++;
            k*=10;
        } else {
            break;
        }
    }
    return digits;
}

std::string operator*(const std::string& s, unsigned int n) {
    std::stringstream out;
    for (unsigned i = 0; i < n; i++)
        out << s;
    return out.str();
}

std::string operator*(unsigned int n, const std::string& s) { return s * n; }

int main(int argc, char** argv) {
    int size, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (argc < 3) {
        std::cout << "Not enough arguments\n";
        return 1;
    }

    std::string first_file(argv[1]);
    std::string second_file(argv[2]);

    std::ifstream first(first_file);
    std::ifstream second(second_file);

    std::ofstream result("result.txt");

    if ((!first.is_open()) || (!second.is_open()) ||
            (!result.is_open())) {
        std::cout << ("Error opening files\n");
        return 1;
    }


    std::string first_string, second_string;

    std::getline(first, first_string);
    std::getline(second, second_string);

    Number first_number = convertToNumber(first_string*2000);
    Number second_number = convertToNumber(second_string*10);

    MPI_Barrier(MPI_COMM_WORLD);
    size_t N = first_number.size() > second_number.size() ? first_number.size() : second_number.size();


    alignToCommonSize(&first_number, N);
    alignToCommonSize(&second_number, N);


    size_t left_index = rank * (N / size);
    size_t right_index = (rank != size - 1) ? (rank + 1) * (N / size) : N;
	
    //std::cout << rank << " " << right_index - left_index << "\n";

    double start = MPI_Wtime();


    int overflow = 0;

    Number with_overflow(right_index - left_index);
    Number without_overflow(right_index - left_index);


    int with = summarize(&with_overflow, first_number, second_number,
        left_index, right_index, 1);
    int without = summarize(&without_overflow, first_number, second_number,
        left_index, right_index, 0);
    

    if (rank != 0) {
        MPI_Recv(&overflow, 1, MPI_INT, rank - 1,
            0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }


    Number answer(right_index - left_index);

    switch (overflow) {
    case 0:
        answer = without_overflow;
        overflow = without;
        break;
    case 1:
        answer = with_overflow;
        overflow = with;
        break;
    }


    if (rank != size - 1) {
       MPI_Send(&overflow, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
    }
    double end = MPI_Wtime() - start;


    if (rank == 0) {
        size_t sizeOfAnswer = answer.size();
        for (size_t i = 0; i < sizeOfAnswer; ++i) {
            first_number[i] = answer[i];
        }
    }


    if (size > 1) {
        if (rank == size - 1) {
            MPI_Send(answer.data(), static_cast<int>(right_index - left_index), MPI_INT, 0, 0, MPI_COMM_WORLD); 
        }
        if (rank == 0) {
            for (int i = 1; i < size; ++i) {
                size_t left = i * (N / size);
                size_t right = (i != size - 1) ? (i + 1) * (N / size) : N;
                MPI_Recv(first_number.data() + left, static_cast<int>(right - left), MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        } else {
            MPI_Send(answer.data(), static_cast<int>(right_index - left_index), MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        MPI_Finalize();
        return 0;
    }



    if (first_number[N - 1] == 0) {
        first_number[N - 1] = MAX_TOKEN;
    }


    result << (first_number.empty() ? 0 : first_number.back());
    for (int i = static_cast<int>(N - 2); i >= 0; --i) {
        size_t sizeOfInt = countDigits(first_number[static_cast<size_t>(i)]);
        while (sizeOfInt < 9) {
            result << "0";
            ++sizeOfInt;
        }
        result << first_number[static_cast<size_t>(i)];
    }
    std::cout << end-start << "\n";
    MPI_Finalize();
    return 0;
}
