#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

// Структура для хранения информации о файле
struct FileInfo {
    char filename[BUFFER_SIZE];
    off_t filesize;
};

// Функция для получения информации о файле (имя и размер)
void getFileInfo(char *filepath, struct FileInfo *fileInfo) {
    struct stat fileStat;
    if (stat(filepath, &fileStat) == 0) {
        strcpy(fileInfo->filename, filepath);
        fileInfo->filesize = fileStat.st_size;
    }
}

// Функция для архивации файлов в указанной директории
void archiveFiles(const char *dirname, FILE *archive) {
    DIR *dir;
    struct dirent *entry;
    struct FileInfo fileInfo;

    // Открываем директорию
    if ((dir = opendir(dirname)) != NULL) {
        // Читаем содержимое директории
        while ((entry = readdir(dir)) != NULL) {
            // Пропускаем текущий и родительский каталоги
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Формируем полный путь к файлу
            char filepath[BUFFER_SIZE];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirname, entry->d_name);

            // Получаем информацию о файле
            getFileInfo(filepath, &fileInfo);

            // Записываем информацию о файле в архив
            fprintf(archive, "%lld||%s||", (long long)fileInfo.filesize, fileInfo.filename);

            // Если файл является директорией, рекурсивно архивируем ее содержимое
            if (S_ISDIR(fileInfo.filesize)) {
                archiveFiles(filepath, archive);
            }
        }
        closedir(dir);
    }
}

// Функция для разархивации файлов из архива
void extractFiles(FILE *archive, const char *outputDir) {
    struct FileInfo fileInfo;
    char filesizeStr[20], filename[BUFFER_SIZE];

    // Читаем информацию из архива и извлекаем файлы
    while (fscanf(archive, "%19[^|]|%[^|]|", filesizeStr, filename) == 2) {
        off_t filesize = atoll(filesizeStr);
        char filepath[BUFFER_SIZE];
        snprintf(filepath, sizeof(filepath), "%s/%s", outputDir, filename);

        // Создаем директории, если они не существуют
        char *last_slash = strrchr(filepath, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
            mkdir(filepath, 0777);
            *last_slash = '/';
        }

        // Создаем файл и копируем данные из архива
        FILE *outputFile = fopen(filepath, "wb");
        if (outputFile != NULL) {
            char buffer[BUFFER_SIZE];
            size_t bytesRead;
            for (off_t bytesRemaining = filesize; bytesRemaining > 0; bytesRemaining -= bytesRead) {
                size_t bytesToRead = (bytesRemaining < BUFFER_SIZE) ? bytesRemaining : BUFFER_SIZE;
                bytesRead = fread(buffer, 1, bytesToRead, archive);
                fwrite(buffer, 1, bytesRead, outputFile);
            }
            fclose(outputFile);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <command> <input_dir> <output_file>\n", argv[0]);
        printf("Commands: archive | extract\n");
        return 1;
    }

    char *command = argv[1];
    char *inputDir = argv[2];
    char *outputFile = argv[3];

    if (strcmp(command, "archive") == 0) {
        // Создаем файл архива
        FILE *archive = fopen(outputFile, "wb");
        if (archive == NULL) {
            perror("Error creating archive file");
            return 1;
        }

        // Архивируем файлы
        archiveFiles(inputDir, archive);

        // Закрываем файл архива
        fclose(archive);

        printf("Archive created successfully: %s\n", outputFile);
    } else if (strcmp(command, "extract") == 0) {
        // Открываем архив для чтения
        FILE *archive = fopen(inputDir, "rb");
        if (archive == NULL) {
            perror("Error opening archive file");
            return 1;
        }

        // Извлекаем файлы из архива
        extractFiles(archive, outputFile);

        // Закрываем файл архива
        fclose(archive);

        printf("Files extracted successfully to: %s\n", outputFile);
    } else {
        printf("Invalid command. Please use 'archive' or 'extract'.\n");
        return 1;
    }

    return 0;
}
