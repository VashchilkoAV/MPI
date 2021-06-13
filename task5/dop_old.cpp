#include <mpi.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#define MILLION 1000*1000*1000

typedef std::vector<size_t> Number;


Number convertToNumber(const std::string& line) {
    Number number;

    for (int i = static_cast<int>(line.length()); i > 0; i-= 9) {
        if (i >= 9) {
            number.push_back(std::stoul(line.substr(
                static_cast<size_t>(i) - 9, 9)));
        } else {
            number.push_back(std::stoul(line.substr(
                0, static_cast<size_t>(i))));
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


int summarize(Number* answer, const Number& first, const Number& second,
        size_t left, size_t right, size_t overflow) {
    for (size_t i = 0; i < right - left; ++i) {
        answer->data()[i] = first[left + i] + second[left + i] + overflow;
        if (answer->data()[i] >= MILLION) {
            overflow = 1;
            answer->data()[i] -= MILLION;
        } else {
            overflow = 0;
        }
    }

    return static_cast<int>(overflow);
}


size_t countDigits(size_t n) {
    size_t digits = 1;
    if (n >=10) {
        digits++;
        if (n >=100) {
            digits++;
            if (n >=1000) {
                digits++;
                if (n >=10000) {
                    digits++;
                    if (n >=100000) {
                        digits++;
                        if (n >=1000000) {
                            digits++;
                            if (n >=10000000) {
                                digits++;
                                if (n >=100000000) {
                                    digits++;
                                    if (n >=1000000000) {
                                        digits++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return digits;
}

std::string operator*(const std::string& s, unsigned int n) {
   std::stringstream out;
    while (n--)
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
        std::cout << "Not enough arguments to the file " << argv[0]
            << std::endl;
        return EXIT_FAILURE;
    }

    std::string first_file = argv[1];
    std::string second_file = argv[2];
    std::ifstream first_stream(first_file);
    std::ifstream second_stream(second_file);

    std::ofstream result("result.txt");

    if ((!first_stream.is_open()) || (!second_stream.is_open()) ||
            (!result.is_open())) {
        std::cout << ("Error opening files\n") << std::endl;
        return EXIT_FAILURE;
    }


    std::string first_string, second_string;
   // double start1 = MPI_Wtime();

    std::getline(first_stream, first_string);
    std::getline(second_stream, second_string);

    first_stream.close();
    second_stream.close();

    Number first_number = convertToNumber(first_string*2000);
    Number second_number = convertToNumber(second_string*10);
   // double end1 = MPI_Wtime() - start1;
   // std::cout << size << " " << end1 << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
    size_t N = first_number.size() > second_number.size() ?
        first_number.size() : second_number.size();


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
		MPI_Send(answer.data(), static_cast<int>(right_index - left_index),
                MPI_INT, 0, 0, MPI_COMM_WORLD);
	  //      std::cout << "send " << rank << " " << answer.size() << "\n"; 
		MPI_Finalize();
		return EXIT_SUCCESS;
	}
        if (rank == 0) {
            for (int i = 1; i < size; ++i) {

                size_t left = i * (N / size);
                size_t right = (i != size - 1) ? (i + 1) * (N / size) : N;
	//	std::cout << "recv " << i << " " << right - left << "\n";
               MPI_Recv(first_number.data() + left,
                    static_cast<int>(right - left), MPI_INT, i,
                    0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        } else {
    	  //  std::cout << "send" << rank << " " << right_index - left_index << " " << answer.size() << "\n";
            MPI_Send(answer.data(), static_cast<int>(right_index - left_index),
                MPI_INT, 0, 0, MPI_COMM_WORLD);
            result.close();
	    MPI_Finalize();
	    return EXIT_SUCCESS;
        }
    }



    if (first_number[N - 1] == 0) {
        first_number[N - 1] = MILLION;
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


    result.close();

    std::cout << size << " " << end << std::endl;

    MPI_Finalize();
    return EXIT_SUCCESS;
}
