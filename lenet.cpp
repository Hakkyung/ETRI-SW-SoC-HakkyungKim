#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<opencv2/opencv.hpp>
#include<iostream>
#include<math.h>
#include<limits.h>

using namespace cv;
using namespace std;



float change(int *depth, int *n, int *m, float ***in_data, float *out_data){

    for(int k=0; k<*depth; k++){    
        for(int i=0; i<*n; i++){
            for(int j=0; j<*m; j++){
                out_data[ (k*(*m)*(*n)) + (i*(*m)) +j] = in_data[k][i][j];
            }
        }
    }
}


float getActivation(int *l, float *in_data){       // Relu function

    for(int x=0; x<*l; x++){
        if(in_data[x]<0){
            in_data[x] = 0;
        }      
    }
}


float matrix_convolution(int *p_depth, int *p_channel, int *p_kernel, int *n, int *m, int *p_stride, float ***in_data, float ****p_weight, float ***out_data){
    //out_data n x m

float sum = 0;
    for(int d = 0; d< *p_depth; d++){

        for(int y = 0; y < *n ; y++){
            for(int x = 0; x < *m; x++){

                for(int k=0; k<*p_channel; k++){        // channel 수
                    for(int i = y*(*p_stride); i< y*(*p_stride) + (*p_kernel); i++){         
                        for(int j = x*(*p_stride); j< x*(*p_stride) + (*p_kernel); j++){               
                            sum += in_data[k][i][j] * p_weight[d][k][i-y*(*p_stride)][j-x*(*p_stride)];
                        }
                    }
                }
                out_data[d][y][x] = sum;
                sum = 0;
            }
        }
    }
}


float max_pooling(int *k, int *p_stride, int *p_kernel, int *n, int *m, float ***in_data, float ***out_data){

    float max = INT_MIN;       // data 가 음수일경우 max = 0  // data min값으로 초기화

    for(int z=0; z <*k; z++){        // depth 수
        // pooled 함수 초기화   // data 가 음수일경우 max = 0  // data min값으로 초기화
        for(int i = 0; i< *n; i++){
            for(int j = 0; j< *m; j++){
                out_data[z][i][j] = 0;
            }
        }

        for(int y = 0; y < *n; y++){             // Convolution 출력값을 kernel, stride에 맞게 pooling
            for(int x = 0; x < *m; x++){
                for(int i = y*(*p_stride); i < *p_kernel + y*(*p_stride); i++){
                    for(int j = x*(*p_stride); j < *p_kernel + x*(*p_stride); j++){
                        if(in_data[z][i][j] > max){
                            max = in_data[z][i][j];
                        }    
                    }    
                }
                out_data[z][y][x] = max;
                max = INT_MIN;
            }
        }
    }
}

// 메인 함수

int main(){

//-----------------------------------------------------------------------------------------------
    // variable of convolution and pooling matrix
    float ***p_data;           // point input data matrix
    // float ****p_weight;         // point filter weight matrix -> conv1, conv2, ip1, ip2
    float ***p_con_data;       // point convolution matrix
    float ***p_pooled;         // pooled data matrix
//-----------------------------------------------------------------------------------------------
    // variable of load weight matrix
    float ****conv1;   // [20][1][5][5]
    float ****conv2;   // [50][20][5][5]
    float **ip1;       // [500][800]
    float **ip2;       // [10][500]
//-----------------------------------------------------------------------------------------------
    // variable of convolution and pooling
    int stride;
    int kernel;
    int depth;      // filter depth
    int con;        // convolution n value
    int com;        // convolution m value
    int pn;         // pooled data n value
    int pm;         // pooled data n value
    int channel;    // number of channel
//-----------------------------------------------------------------------------------------------
    // variable of read weight part
    char cha;       // 쓰레기 문자
    char dump[12];  // 쓰레기 문자열
    float check;   
//-----------------------------------------------------------------------------------------------
    // variable of fullyconnected
    float *p_fully;
    float *p_fully2;
    int idata;
    int odata;
//-----------------------------------------------------------------------------------------------
    // load weight data
    FILE *fp;
    fp = fopen("/home/socmgr/khk/0726/ordered_weights.txt","r");

    if(fp == NULL){
        printf("Read Error\n");
        return 0;
    }

    fscanf(fp,"%s",dump);
    fscanf(fp,"%s",dump);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%c",&cha);

    fscanf(fp,"%d",&depth);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%d",&channel);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%d",&kernel);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%d",&kernel);

    fscanf(fp,"%s",dump);
    fscanf(fp,"%s",dump);

    fscanf(fp,"%c",&cha);  
    fscanf(fp,"%c",&cha); 
    fscanf(fp,"%c",&cha);   
    fscanf(fp,"%c",&cha); 
    fscanf(fp,"%c",&cha);  
        
//-----------------------------------------------------------------------------------------------
    // load image file
    Mat image;          // image 변수선언 
    image = imread("./number/9.png", IMREAD_GRAYSCALE);       // 이미지 파일 불러오기 

    int in = image.rows;        // input data n value
    int im = image.cols;        // input data m value

    if (image.empty())      // 파일 확인
    {
        cout << "Could not open of find the image" << endl;
        return -1;
    }
//-----------------------------------------------------------------------------------------------
    // calculation
    stride = 1; // stride는 weight.json file에 없음
    con = (in-kernel)/stride + 1;
    com = (im-kernel)/stride + 1;
//-----------------------------------------------------------------------------------------------
    // Declare point
    int *p_depth;
    p_depth = &depth;
    int *p_channel;
    p_channel = &channel;

    int *p_in;
    p_in = &in;
    int *p_im;
    p_im = &im;
    int *p_con;
    p_con = &con;
    int *p_com;
    p_com = &com;
    int *p_pn;
    p_pn = &pn;
    int *p_pm;
    p_pm = &pm;

    int *p_stride;
    p_stride = &stride;
    int *p_kernel;
    p_kernel = &kernel;

    int *p_odata;
    p_odata = &odata;
//-----------------------------------------------------------------------------------------------
    // 동적메모리할당

    // input data matrix in x in
    p_data = (float***)malloc(channel*sizeof(float**));
    for(int i=0; i<channel; i++){   
        *(p_data+i) = (float**)malloc(in*sizeof(float*));
        for(int j=0; j<in; j++){
            *(*(p_data+i)+j) = (float*)malloc(im*sizeof(float));
        }
    }
 
    // convolution matrix 
    p_con_data = (float***)malloc(depth*sizeof(float**));
    for(int i=0; i<depth; i++){   
        *(p_con_data+i) = (float**)malloc(con*sizeof(float*));
        for(int j=0; j<con; j++){
            *(*(p_con_data+i)+j) = (float*)malloc(com*sizeof(float));
        }
    }

    // conv1 weight data matrix
    conv1 = (float****)malloc(depth*sizeof(float***));
    for(int k=0; k<depth; k++){
        *(conv1+k) = (float***)malloc(channel*sizeof(float**));
        for(int i=0; i<channel; i++){   
            *(*(conv1+k)+i) = (float**)malloc(kernel*sizeof(float*));
            for(int j=0; j<kernel; j++){
                *(*(*(conv1+k)+i)+j) = (float*)malloc(kernel*sizeof(float));
            }
        }
    }
//-----------------------------------------------------------------------------------------------
    // input image data
    for(int i=0; i<in; i++){
        for(int j=0; j<im; j++){
            p_data[0][i][j] = image.at<uchar>(i,j);
        }
    }
//-----------------------------------------------------------------------------------------------
    // load conv1 weight data
    for(int d=0; d<depth; d++){
        for(int c=0; c<channel; c++){
            for(int i=0; i<kernel; i++){
                for(int j=0; j<kernel; j++){
                    fscanf(fp,"%f",&check);
                    conv1[d][c][i][j] = check;
                    fscanf(fp,"%c",&cha);
                }          
                fscanf(fp,"%c",&cha);
                fscanf(fp,"%c",&cha);
                fscanf(fp,"%c",&cha);
            }      
            fscanf(fp,"%c",&cha);
            fscanf(fp,"%c",&cha);
        }
        fscanf(fp,"%c",&cha);
        fscanf(fp,"%c",&cha);
    }
//-----------------------------------------------------------------------------------------------
    // conv1
    matrix_convolution(p_depth, p_channel, p_kernel, p_con, p_com, p_stride, p_data, conv1, p_con_data);
    printf("\nconv1\n");
    printf("depth : %d, channel : %d, kernel : %d, rows : %d, cols : %d, stride : %d\n", depth, channel, kernel, con, com, stride);

//-----------------------------------------------------------------------------------------------
    // free image data
        for(int i=0; i<channel; i++){
            for(int j=0; j<in; j++){
                free(p_data[i][j]);
            }
        }
        free(p_data);
    // free conv1 weight data
    for(int d=0; d<depth; d++){
        for(int c=0; c<channel; c++){
            for(int i=0; i<kernel; i++){
                free(conv1[d][c][i]);
            }
        }
    }
    free(conv1);
//-----------------------------------------------------------------------------------------------
    // calculation
    stride = 2; // stride는 weight.json file에 없음
    kernel = 2; // pooling kernel은 weight.json file에 없음
    pn = (con-kernel)/stride + 1;
    pm = (com-kernel)/stride + 1;
//-----------------------------------------------------------------------------------------------
    // 동적메모리할당

    // Pooled data matrix
    p_pooled = (float***)malloc(depth*sizeof(float**));
    for(int i=0; i<depth; i++){   
        *(p_pooled+i) = (float**)malloc(pn*sizeof(float*));
        for(int j=0; j<pn; j++){
            *(*(p_pooled+i)+j) = (float*)malloc(pm*sizeof(float));
        }
    }

//-----------------------------------------------------------------------------------------------
    
    // pool1
    max_pooling(p_depth, p_stride, p_kernel, p_pn, p_pm, p_con_data, p_pooled); 
    printf("\npooling1\n");
    printf("depth : %d, channel : %d, kernel : %d, rows : %d, cols : %d, stride : %d\n", depth, channel, kernel, pn, pm, stride);
    
//-----------------------------------------------------------------------------------------------
    // load conv2 weight data
    fscanf(fp,"%s",dump);
    fscanf(fp,"%s",dump);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%c",&cha);

    fscanf(fp,"%d",&depth);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%d",&channel);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%d",&kernel);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%d",&kernel);

    fscanf(fp,"%s",dump);
    fscanf(fp,"%s",dump);

    fscanf(fp,"%c",&cha); 
    fscanf(fp,"%c",&cha);  
    fscanf(fp,"%c",&cha); 
    fscanf(fp,"%c",&cha);  
    fscanf(fp,"%c",&cha);
//-----------------------------------------------------------------------------------------------
    // calculation
    stride = 1; // stride는 weight.json file에 없음
    con = (pn-kernel)/stride + 1;
    com = (pn-kernel)/stride + 1;    

//-----------------------------------------------------------------------------------------------
    // 동적메모리 할당

    //point convolution matrix 
    p_con_data = (float***)malloc(depth*sizeof(float**));
    for(int i=0; i<depth; i++){   
        *(p_con_data+i) = (float**)malloc(con*sizeof(float*));
        for(int j=0; j<con; j++){
            *(*(p_con_data+i)+j) = (float*)malloc(com*sizeof(float));
        }
    }
    // conv2 weight data matrix
    conv2 = (float****)malloc(depth*sizeof(float***));
    for(int k=0; k<depth; k++){
        *(conv2+k) = (float***)malloc(channel*sizeof(float**));
        for(int i=0; i<channel; i++){   
            *(*(conv2+k)+i) = (float**)malloc(kernel*sizeof(float*));
            for(int j=0; j<kernel; j++){
                *(*(*(conv2+k)+i)+j) = (float*)malloc(kernel*sizeof(float));
            }
        }
    }
//-----------------------------------------------------------------------------------------------
    // load conv2 weight data
    for(int d=0; d<depth; d++){
        for(int c=0; c<channel; c++){
            for(int i=0; i<kernel; i++){
                for(int j=0; j<kernel; j++){
                    fscanf(fp,"%f",&check);
                    conv2[d][c][i][j] = check;
                    fscanf(fp,"%c",&cha);
                }
                fscanf(fp,"%c",&cha);
                fscanf(fp,"%c",&cha);
                fscanf(fp,"%c",&cha);
            }
            fscanf(fp,"%c",&cha);
            fscanf(fp,"%c",&cha);
        }
        fscanf(fp,"%c",&cha);
        fscanf(fp,"%c",&cha);
    }
//-----------------------------------------------------------------------------------------------
    
    //conv2
    matrix_convolution(p_depth, p_channel, p_kernel, p_con, p_com, p_stride, p_pooled, conv2, p_con_data);
    printf("\nconv2\n");
    printf("depth : %d, channel : %d, kernel : %d, rows : %d, cols : %d, stride : %d\n", depth, channel, kernel, con, com, stride);

//-----------------------------------------------------------------------------------------------
// free conv2 weight data
    for(int d=0; d<depth; d++){
        for(int c=0; c<channel; c++){
            for(int i=0; i<kernel; i++){
                free(conv2[d][c][i]);
            }
        }
    }
    free(conv2);
//-----------------------------------------------------------------------------------------------
    // calculation
    stride = 2; // stride는 weight.json file에 없음
    kernel = 2; // pooling kernel은 weight.json file에 없음
    pn = (con-kernel)/stride + 1;
    pm = (com-kernel)/stride + 1;
//-----------------------------------------------------------------------------------------------
    // 동적메모리할당
    
    //point Pooled data matrix
    p_pooled = (float***)malloc(depth*sizeof(float**));
    for(int i=0; i<depth; i++){   
        *(p_pooled+i) = (float**)malloc(pn*sizeof(float*));
        for(int j=0; j<pn; j++){
            *(*(p_pooled+i)+j) = (float*)malloc(pm*sizeof(float));
        }
    }
//-----------------------------------------------------------------------------------------------
   
    // pool2
    max_pooling(p_depth, p_stride, p_kernel, p_pn, p_pm, p_con_data, p_pooled); 
    printf("\npooling2\n");
    printf("depth : %d, channel : %d, kernel : %d, rows : %d, cols : %d, stride : %d\n", depth, channel, kernel, pn, pm, stride);

    for(int i = 0; i<depth; i++){
        for(int j =0; j<pn; j++){
            for(int k=0; k<pm; k++){
                printf("%lf ",p_pooled[i][j][k]);
            }
            printf("\n");
        }
        printf("\n");printf("\n");
    }

//-----------------------------------------------------------------------------------------------
    //3차원 배열을 1차원 배열로 만들어준다.
    float *p_loose;
    p_loose = (float*)malloc((depth*pn*pm)*sizeof(float));        //depth*pn*pm = indata
    change(p_depth, p_pn, p_pm, p_pooled, p_loose);
//-----------------------------------------------------------------------------------------------
    // free convolution data   
    for(int i=0; i<depth; i++){   
        for(int j=0; j<con; j++){
            free(p_con_data[i][j]);
        }
    }
    free(p_con_data);
    // free pooling data
    for(int i=0; i<depth; i++){   
        for(int j=0; j<pn; j++){
            free(p_pooled[i][j]);
        }
    }
    free(p_pooled);
//-----------------------------------------------------------------------------------------------
    //fully connected
//-----------------------------------------------------------------------------------------------
    // load ip1 data
    fscanf(fp,"%s",dump);
    fscanf(fp,"%s",dump);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%c",&cha);

    fscanf(fp,"%d",&odata);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%d",&idata);

    fscanf(fp,"%s",dump);
    fscanf(fp,"%s",dump);

    fscanf(fp,"%c",&cha);  
    fscanf(fp,"%c",&cha);  
    fscanf(fp,"%c",&cha); 
    fscanf(fp,"%c",&cha); 
    fscanf(fp,"%c",&cha);  

    printf("\nip1\n");
    printf("odata : %d idata : %d\n",odata, idata);
//-----------------------------------------------------------------------------------------------
    // 동적메모리할당
    ip1 = (float**)malloc(odata*sizeof(float*));
    for(int i=0; i<odata; i++){
        *(ip1+i)= (float*)malloc(idata*sizeof(float));
    }

    p_fully = (float*)malloc(odata*sizeof(float)); 
//-----------------------------------------------------------------------------------------------
    // load ip1 data
    for(int i=0; i<odata; i++){
        for(int j=0; j<idata; j++){
            fscanf(fp,"%f",&check);
            ip1[i][j] = check;
            fscanf(fp,"%c",&cha);
        }
        fscanf(fp,"%c",&cha);
        fscanf(fp,"%c",&cha);
        fscanf(fp,"%c",&cha);
    }
//-----------------------------------------------------------------------------------------------
    // ip1
    for(int i=0; i<odata; i++){
        for(int j=0; j<idata; j++){
            p_fully[i] +=  p_loose[j]*ip1[i][j];
        }
    }   

    FILE *fpi1;
    fpi1 = fopen("/home/socmgr/khk/0726/check/fpi1.txt","w");

    if(fpi1 == NULL){
        printf("Read Error\n");
        return 0;
    }
    for(int i=0; i<odata; i++){
        fprintf(fpi1,"%lf\n",p_fully[i]);
    }

    fclose(fpi1);

//-----------------------------------------------------------------------------------------------
    // free ip1 data
    for(int i=0; i<odata; i++){
        free(ip1[i]);
    }
    free(ip1);

    // free p_loose
    free(p_loose);
//-----------------------------------------------------------------------------------------------
    //ReLU         
    getActivation(p_odata, p_fully);
//-----------------------------------------------------------------------------------------------
    // load ip2 data
    fscanf(fp,"%s",dump);
    fscanf(fp,"%s",dump);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%c",&cha);

    fscanf(fp,"%d",&odata);
    fscanf(fp,"%c",&cha);
    fscanf(fp,"%d",&idata);

    fscanf(fp,"%s",dump);
    fscanf(fp,"%s",dump);

    fscanf(fp,"%c",&cha);   
    fscanf(fp,"%c",&cha);  
    fscanf(fp,"%c",&cha);




    printf("\nip2\n");
    printf("odata : %d idata : %d\n",odata, idata);
//-----------------------------------------------------------------------------------------------
    // 동적메모리할당
    ip2 = (float**)malloc(odata*sizeof(float*));
    for(int i=0; i<odata; i++){
        *(ip2+i)= (float*)malloc(idata*sizeof(float));
    }

    p_fully2 = (float*)malloc(odata*sizeof(float)); 
//-----------------------------------------------------------------------------------------------
    // load ip2 data
    for(int i=0; i<odata; i++){
        for(int j=0; j<idata; j++){
            fscanf(fp,"%f",&check);
            ip2[i][j] = check;
            fscanf(fp,"%c",&cha);
        }
        fscanf(fp,"%c",&cha);
        fscanf(fp,"%c",&cha);
        fscanf(fp,"%c",&cha);
    }
    fclose(fp);
//-----------------------------------------------------------------------------------------------
    // ip2
    
    for(int i=0; i<odata; i++){
        p_fully2[i] =0 ;
    }  
    

    for(int i=0; i<odata; i++){
        for(int j=0; j<idata; j++){
            p_fully2[i] +=  p_fully[j]*ip2[i][j];
        }
    }  
//-----------------------------------------------------------------------------------------------
    FILE *fpi2;
    fpi2 = fopen("/home/socmgr/khk/0726/check/fpi2.txt","w");

    if(fpi2 == NULL){
        printf("Read Error\n");
        return 0;
    }
    for(int i=0; i<odata; i++){
        fprintf(fpi2,"%lf\n",p_fully2[i]);
    }

    fclose(fpi2);
//-----------------------------------------------------------------------------------------------
    // free ip2 data
    for(int i=0; i<odata; i++){
        free(ip2[i]);
    }   
    free(ip2);
//-----------------------------------------------------------------------------------------------

    printf("\n\n");
    for(int i=0; i<odata; i++){
        printf("%d : %f\n", i,p_fully2[i]);
    }

//-----------------------------------------------------------------------------------------------
    //softmax

    float isum = 0;
    float imax = 0;

    for(int i=0; i<odata; i++){
        if(imax < p_fully2[i]){
            imax = p_fully2[i];
        }
    }

    printf("\nsoft_max\n");
    for(int i=0; i<odata; i++){
        isum += exp(p_fully2[i]-imax);
    }
    for(int i=0; i<odata; i++){
        p_fully2[i] = exp(p_fully2[i]-imax)/isum;
        printf("%d, %f\n",i,p_fully2[i]);
    }

//-----------------------------------------------------------------------------------------------
    
    free(p_fully);
    free(p_fully2);

//-----------------------------------------------------------------------------------------------
}
