#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>
#include <pthread.h>


#define MAX_BUFFER_SIZE 51200

const unsigned n_RSA = 19303;
const unsigned d_RSA = 13595;

static void ParseImage(int numPixelsToProcess);
unsigned long long decyptRSA(unsigned long long Pixel, unsigned long long d, unsigned long long n);
char resultadoFinal[4];

int client_rank, client_size;
MPI_Comm client_comm;
// Definir el tipo de dato derivado para struct Pixel
MPI_Datatype pixelType;
MPI_Datatype pixelTypeFloat;

// Definir la estructura para representar un píxel
struct Pixel {
    int r;
    int g;
    int b;
};

struct resultPixel {
    float r;
    float g;
    float b;
};



unsigned long long decyptRSA(unsigned long long Pixel, unsigned long long d, unsigned long long n) {
    // Función para calcular la exponenciación modular (a^b % n)
   unsigned long long newP = 1;
    while (d > 0) {
        if (d % 2 == 1) {
            newP = (newP * Pixel) % n;
        }
        Pixel = (Pixel * Pixel) % n;
        d /= 2;
    }
    return newP;
}
// Función para calcular el porcentaje de R, G y B en una lista de píxeles
struct resultPixel calcularPorcentajeRGB(int pixels[][3], int numPixels) {
    // Variables para acumular los valores de R, G y B
    float totalR = 0;
    float totalG = 0;
    float totalB = 0;

    struct resultPixel result;


    printf("NumPixel: %d\n",numPixels);

    // Iterar a través de la lista de píxeles
    for (int i = 0; i < numPixels; i++) {
     
        // Asegurarse de que los valores estén en el rango válido (0-255)
        double r = (decyptRSA(pixels[i][0],d_RSA,n_RSA) < 0) ? 0 : ((decyptRSA(pixels[i][0],d_RSA,n_RSA) > 255) ? 255 : decyptRSA(pixels[i][0],d_RSA,n_RSA));
        double g = (decyptRSA(pixels[i][1],d_RSA,n_RSA) < 0) ? 0 : ((decyptRSA(pixels[i][1],d_RSA,n_RSA) > 255) ? 255 : decyptRSA(pixels[i][1],d_RSA,n_RSA));
        double b = (decyptRSA(pixels[i][2],d_RSA,n_RSA) < 0) ? 0 : ((decyptRSA(pixels[i][2],d_RSA,n_RSA) > 255) ? 255 : decyptRSA(pixels[i][2],d_RSA,n_RSA));



        // Acumular los valores normalizados de R, G y B
        totalR += r;
        totalG += g;
        totalB += b;
    }
    float porcentajeR = (totalR / (totalR+totalG+totalB))*10;
    float porcentajeG = (totalG / (totalR+totalG+totalB))*10;
    float porcentajeB = (totalB / (totalR+totalG+totalB))*10;

    result.r = porcentajeR;
    result.g = porcentajeG;
    result.b = porcentajeB;

    return result;

}

const char CalcularResultadoFinal(float Mr, float Mb, float Mg, float Sr, float Sb, float Sg) {
    float R, G, B;
    int R2, G2, B2;
    char R3[2];  // Aumentado el tamaño
    char G3[2];  // Aumentado el tamaño
    char B3[2];  // Aumentado el tamaño

    R = (Mr + Sr) / 2;
    G = (Mg + Sg) / 2;
    B = (Mb + Sb) / 2;

    R2 = round(R);
    G2 = round(G);
    B2 = 10 - (R2 + G2);

    snprintf(R3, sizeof(R3), "%d", R2);
    snprintf(B3, sizeof(B3), "%d", B2);
    snprintf(G3, sizeof(G3), "%d", G2);

    resultadoFinal[0] = R2 + '0';  
    resultadoFinal[1] = G2 + '0';  
    resultadoFinal[2] = B2 + '0';  
    resultadoFinal[3] = '\0'; 

    printf("Porcentaje de Rojo %s\n", R3);
    printf("Porcentaje de Verde %s\n", G3);
    printf("Porcentaje de Azul %s\n", B3);


}


static void ParseImage(int numPixelsToProcess){

    
    double start_time, end_time;

    // Iniciar el tiempo de ejecución
    start_time = MPI_Wtime();

    // Abrir el archivo en modo de lectura
    FILE *archivo = fopen("archivo_recibido.txt", "r");

    // Verificar si se pudo abrir el archivo
    if (archivo == NULL) {
        fprintf(stderr, "No se pudo abrir el archivo.\n");
        return 1;
    }

    // Obtener el tamaño del archivo
    fseek(archivo, 0, SEEK_END);
    long fileSize = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    // Asignar dinámicamente el buffer con el tamaño del archivo + 1 para el terminador nulo
    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error de asignación de memoria para el buffer.\n");
        fclose(archivo);
        return 1;
    }

    // Leer el archivo completo en el buffer
    fread(buffer, 1, fileSize, archivo);

    // Agregar un terminador nulo al final del buffer
    buffer[fileSize] = '\0';

    // Variables para almacenar los píxeles
    struct Pixel *pixels = malloc(fileSize);  // Tamaño suficiente para todos los píxeles
    int numPixels = 0;

    // Usar strtok para dividir las líneas por '/'
    char *token = strtok(buffer, "/");

    while (token != NULL) {

        if(numPixels >= numPixelsToProcess && numPixelsToProcess != 0){
            printf("Entra al break");
            break;
        }
        // Parsear los componentes R, G, B
        int r, g, b;
        sscanf(token, "%d,%d,%d", &r, &g, &b);

        // Almacenar el píxel en el array
        pixels[numPixels].r = r;
        pixels[numPixels].g = g;
        pixels[numPixels].b = b;

        // Incrementar el contador de píxeles
        numPixels++;

        // Obtener el siguiente píxel
        token = strtok(NULL, "/");
    }

    // Liberar la memoria asignada dinámicamente para el buffer
    free(buffer);

    // Cerrar el archivo después de leerlo
    fclose(archivo);



    int mitad = numPixels / 2 + (numPixels % 2);

    int contA=0;
    int contB=0;
    int contTotal=0;

    struct Pixel *pixelsA = malloc(fileSize);
    struct Pixel *pixelsB = malloc(fileSize);


    for (int i = 0; i < numPixels; i++) {
        
        if(i<= mitad){
            pixelsA[contA].r = pixels[contTotal].r;
            pixelsA[contA].g = pixels[contTotal].g;
            pixelsA[contA].b = pixels[contTotal].b;

            contTotal++;
            contA++;

        }else{
            pixelsB[contB].r = pixels[contTotal].r;
            pixelsB[contB].g = pixels[contTotal].g;
            pixelsB[contB].b = pixels[contTotal].b;

            contTotal++;
            contB++;
        }
    }

    int buffer_size = sizeof(struct Pixel) * contB;
    char bufferRPC[buffer_size];


    char caracter[10];
    snprintf(caracter, sizeof(caracter), "%d", contB);


    MPI_Send(caracter, strlen(caracter)+1, MPI_CHAR, 1, 0, client_comm);

// Enviar todo el array usando MPI_Send
    MPI_Send(pixelsB, contB, pixelType, 1, 0, MPI_COMM_WORLD);

    printf("Proceso 0 envió el array de estructuras Pixel.\n");

    struct resultPixel result;


    result=calcularPorcentajeRGB(pixelsA,contA);


    struct resultPixel resultSlave [100];

    MPI_Recv(resultSlave, 1, pixelTypeFloat, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Llamar a la función y almacenar el resultado en una variable
   
    CalcularResultadoFinal(resultSlave[0].r, resultSlave[0].b, resultSlave[0].g, result.r, result.b, result.g);

    // Imprimir la variable
    printf("Resultado Final: %s\n", resultadoFinal);
    


    // Liberar la memoria asignada dinámicamente para los píxeles
    free(pixels);
    free(pixelsA);
    free(pixelsB);

    end_time = MPI_Wtime();

    // Imprimir el tiempo de ejecución
    printf("Tiempo de ejecución: %f segundos\n", end_time - start_time);


}

int main(int argc, char** argv) 
{
    MPI_Init(&argc, &argv);

    
    MPI_Comm_rank(MPI_COMM_WORLD, &client_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &client_size);

    if (client_size < 1) {
        printf("Este programa requiere al menos 1 proceso para el cliente.\n");
        MPI_Finalize();
        return 1;
    }

    
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    int blocklengths[3] = {1, 1, 1};
    MPI_Aint offsets[3];

    offsets[0] = offsetof(struct Pixel, r);
    offsets[1] = offsetof(struct Pixel, g);
    offsets[2] = offsetof(struct Pixel, b);

    MPI_Type_create_struct(3, blocklengths, offsets, types, &pixelType);
    MPI_Type_commit(&pixelType);

    //Extructura para porcentajes tipo float

    MPI_Datatype types2[3] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT};
    MPI_Aint offsets2[3];


    offsets2[0] = offsetof(struct resultPixel, r);
    offsets2[1] = offsetof(struct resultPixel, g);
    offsets2[2] = offsetof(struct resultPixel, b);

    MPI_Type_create_struct(3, blocklengths, offsets2, types2, &pixelTypeFloat);
    MPI_Type_commit(&pixelTypeFloat);


    // Crear un comunicador para el cliente
    
    MPI_Comm_split(MPI_COMM_WORLD, 0, client_rank, &client_comm);


    MPI_Barrier(client_comm);


    if (client_rank == 0) {
        int bytesToAnalyze = 10000;

        ParseImage(bytesToAnalyze);

    }else if (client_rank == 1) {

        
         char hostname[100];

        if (gethostname(hostname, sizeof(hostname)) == 0) {
            printf("Hostname: %s\n", hostname);
        }
    

        char pixelesRecibidos[100];
        MPI_Recv(pixelesRecibidos, 100, MPI_CHAR, 0, 0, client_comm, MPI_STATUS_IGNORE);

        int pixelesRecibidosINT=atoi(pixelesRecibidos);
        printf("Pixeles= %d\n", pixelesRecibidosINT);


        struct Pixel receivedPixels[pixelesRecibidosINT];
        MPI_Recv(receivedPixels, pixelesRecibidosINT, pixelType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        printf("Pixeles recibidos por el nodo slave\n");


        struct resultPixel result;
        struct resultPixel *result2=malloc(30);
        result=calcularPorcentajeRGB(receivedPixels,pixelesRecibidosINT);


        result2[0].r = result.r;
        result2[0].g = result.g;
        result2[0].b = result.b;

      



        MPI_Send(result2, 1, pixelTypeFloat, 0, 0, MPI_COMM_WORLD);




    }

    MPI_Finalize();
    

    return 0;
}
