#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <bits/stdc++.h>

#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define INF 999999999

const char help_message[] = "APA Module (asynchronous version)\n\n" 
                            "-h, --help: show this message\n"
                            "-ns, --noscore: execution without the initial heuristic score\n"
                            "-t, --trim: execution with a heuristic score generated by trimmed sequences\n"
                            "-f, --full: execution with a heuristic score generated by full sequences\n";


using namespace std;

void trim(string file1, string file2) {
    string file1_aux = file1;
    string file2_aux = file2;

    string cropped_file1 = file1.substr(file1.find_last_of("/") + 1);
    string cropped_file2 = file1.substr(file2.find_last_of("/") + 1);

    // Executa o trim no arquivo 1 e insere em trimmed_bases
    string cmd = "awk 'NR==1, NR==29538' " + file1 + " > " + "trimmed_bases/" + cropped_file1;
    system(cmd.c_str());

    // Executa o trim no arquivo 2 e insere em trimmed_bases
    cmd = "awk 'NR==1, NR==29538' " + file2 + " > " + "trimmed_bases/" + cropped_file2;
    system(cmd.c_str());
}

int findScore() {
    ifstream file;
    file.open("output");
    string line;
    int score = -INF;
    while (!file.eof() && score == -INF)
    {
        getline(file, line);
        if (line.find(" Score =") != string::npos)
        {
            int pos = line.find("(");
            int pos1 = line.find(")");
            score = atoi(line.substr(pos + 1, pos1 - pos - 1).c_str());
            cout << "Best Score gerado pelo BLASTn:" << score << endl;
        }
    }
    if (score == -INF)
    {
        cout << "Não foi possível gerar um escore pelo BLASTn. Foi aplicado o escore padrão -INF." << endl;
    }
    file.close();
    return score;
}

void clearShm(int sharedMemID) {
    if (sharedMemID != -1) {
        // Kill proc shared memory
        if ((shmctl(sharedMemID, IPC_RMID, NULL)) == -1) {
          printf("Shared memory remove erro.\n");
          if (E2BIG == errno)
            printf("E2BIG");
          if (EACCES == errno)
            printf("EACCES");
          if (EIDRM == errno)
            printf("EIDRM");
          if (EINTR == errno)
            printf("EINTR");
          if (EINVAL == errno)
            printf("EINVAL");
          if (ENOMSG == errno)
            printf("ENOMSG");
        }
  }
}

void removeSharedMem(int sharedMemID) {
    if (sharedMemID != -1) {
    // Kill proc shared memory
    if ((shmctl(sharedMemID, IPC_RMID, NULL)) == -1) {
      printf("Shared memory remove erro.\n");
      if (E2BIG == errno)
        printf("E2BIG");
      if (EACCES == errno)
        printf("EACCES");
      if (EIDRM == errno)
        printf("EIDRM");
      if (EINTR == errno)
        printf("EINTR");
      if (EINVAL == errno)
        printf("EINVAL");
      if (ENOMSG == errno)
        printf("ENOMSG");
    }
  }
}

int execute(int option, string file1, string file2) {

    struct shared_mem {
        bool isReady;
        int blast_score;
    };
    shared_mem sharedMem;
    shared_mem *sharedMemPtr;

    int sharedMemID;
    int CUDAProcessID;
    int childReturn;

    sharedMem.isReady = false;
    sharedMem.blast_score = -INF;

    if ((sharedMemID = shmget(0x706964, sizeof(struct shared_mem) , IPC_CREAT | 0777)) < 0) {
        printf("Modulo APA: Error creating shared meemory %d!\n", 0x706964);
        clearShm(sharedMemID);
        exit(EXIT_FAILURE);
    }
    sharedMemPtr = (struct shared_mem *)shmat(sharedMemID, (char *)0, 0);
    if (sharedMemPtr == (struct shared_mem *)-1) {
        printf("Modulo APA:\n");
        printf("Error in attach!\n");
        clearShm(sharedMemID);
        exit(EXIT_FAILURE);
    }
    *sharedMemPtr = sharedMem;

    string seq1 = file1, seq2 = file2;

    if ((CUDAProcessID = fork()) < 0) {
        printf("Erro no fork\n");
        clearShm(sharedMemID);
        exit(EXIT_FAILURE);
    }
    if (CUDAProcessID == 0) {
        if (execl("masa-cudalign/cudalign", "cudalign", "--clear", "--ram-size=500M", "--disk-size=2G", 
                  "--opening-score=-999999999", file1.c_str(), file2.c_str(), NULL) == -1) {
            printf("Erro no execl para o programa masa-cudalign/cudalign\n");
        }            
    }


    switch (option)
    {
    case 0:

        printf("trim first\n");
        seq1 = "tes1";
        seq2 = "tes2";
    case 1:
        printf("Executa o CUDAlign\n");
        printf("Executada o Blastn\n");
        printf("Envia o resultado do Blastn para o CUDAlign\n");
        printf("Epera a finalização do CUDAlign no wait\n");
        break;
    case 2:
        printf("Executa o CUDAlign e espera ele finalizar\n");
        break;
    default:
        return -1;
        break;
    }

    return 0;

}

int parseInput(int argc, char *argv[]) {

    if (argc != 4 && argc != 2) {
        printf("Invalid number of arguments\n");
        printf("%s", help_message);
        return -1;
    }

    string option = argv[1];

    if (option == "-h" || option == "--help") {
        printf("%s", help_message);
    }
    else if (option == "-t" || option == "--trim") {
        execute(0, "teste1", "teste2");
    }
    else if (option == "-f" || option == "--full") {
        execute(1, "teste1", "teste2");
    }
    else if (option == "-ns" || option == "--noscore") {
        execute(2, "teste1", "teste2");
    }
    else {
        printf("Invalid arguments\n");
        printf("%s", help_message);        
    }


    return 0;
}

int main(int argc, char *argv[]) {

    parseInput(argc, argv);

    return 0;
}