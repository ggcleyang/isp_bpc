//
// Created by ly on 2022/4/8.
//

#include "isp_bpc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


void bubble_sort(int16* data,uint8 len){
    //ascend
    for(uint8 i =0;i<len-1;i++){
        for(uint8 j=0;j<len-i-1;j++){
            if(data[j]>data[j+1]){
                int16 temp =data[j];
                data[j]=data[j+1];
                data[j+1]=temp;
            }
        }
    }
    return;
}
void reshape(uint16* Input,uint16**Output,uint16 img_width,uint16 img_height){

    uint16 m = img_width;
    uint16 n = img_height;
    for(uint16 v = 0;v <n;v++){
        for(uint16 h=0;h<m;h++){
            Output[v][h]=Input[v*m+h];
        }
    }
    return;
}
void generate_gradient(uint16** Input,uint16** gradient_img,uint16 w,uint16 h){

    char *src_img = "D:\\leetcode_project\\src_img.bmp";
    char *gradient_map = "D:\\leetcode_project\\gradient_map_img.bmp";
    char *interpolation_g = "D:\\leetcode_project\\interpolation_g_img.bmp";
    char *gauss_img = "D:\\leetcode_project\\gauss_img.bmp";

    uint8 gauss_kernel3x3[3][3] = {{1,2,1},{2,4,2},{1,2,1}};// 1/16
    int8 sobel_x_3x3[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
    int8 sobel_y_3x3[3][3] = {{1,2,1},{0,0,0},{-1,-2,-1}};

   //use channel-G to generate gradient map
   //interpolation G
   for(uint16 y=1;y<h-1;y++){
       for(uint16 x=1;x<w-1;x++){
           if((BPRG == bayerPattLUT[0][y & 0x1][x & 0x1])||(BPBG == bayerPattLUT[0][y & 0x1][x & 0x1])){
               gradient_img[y][x] =  (Input[y-1][x]+Input[y+1][x] +Input[y][x-1]+ Input[y][x+1])>>2;
           }
           else{
               gradient_img[y][x] =Input[y][x];
           }
       }
   }
    //singleChannel2BMP(gradient_img,w,h,interpolation_g);
   //gaussian filter
    for (uint16 y1 = 1; y1 < h-1; y1++) {
        for (uint16 x1 = 1; x1 < w-1; x1++) {
            uint16 sum = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    uint16 pixel_value = gradient_img[y1+ky][x1+kx];
                    sum += gauss_kernel3x3[ky+1][kx+1] * pixel_value >>4;
                }
            }
            gradient_img[y1][x1] = CLIP3(sum,0,4095);
        }
    }
    //singleChannel2BMP(gradient_img,w,h,gauss_img);
   //sobel
    int16** sobel_x = (int16**)malloc(h*sizeof(int16*));
    for(uint16 v1=0;v1 <h;v1++){
        sobel_x[v1] = (int16*) malloc(w*sizeof(int16));
    }
    memset(&sobel_x[0][0],0,sizeof(sobel_x));

    int16** sobel_y = (int16**)malloc(h*sizeof(int16*));
    for(uint16 v1=0;v1 <h;v1++){
        sobel_y[v1] = (int16*) malloc(w*sizeof(int16));
    }
    memset(&sobel_y[0][0],0,sizeof(sobel_y));

    for (uint16 y2 = 1; y2 < h-1; y2++) {
        for (uint16 x2 = 1; x2 < w-1; x2++) {
            int16 sum2 = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    uint16 pixel_value2 = gradient_img[y2+ky][x2+kx];
                    sum2 += sobel_x_3x3[ky+1][kx+1] * pixel_value2;
                }
            }
            sobel_x[y2][x2] = sum2;

        }
    }
    for (uint16 y3 = 1; y3 < h-1; y3++) {
        for (uint16 x3 = 1; x3 < w-1; x3++) {
            int16 sum3 = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    uint16 pixel_value3 = gradient_img[y3+ky][x3+kx];
                    sum3 += sobel_y_3x3[ky+1][kx+1] * pixel_value3;
                }
            }
            sobel_y[y3][x3] = sum3;

        }
    }
    //gradient map
    for (uint16 y4 = 0; y4 < h; y4++) {
        for (uint16 x4 = 0; x4 < w; x4++) {
            uint16 temp = CLIP3(abs(sobel_x[y4][x4])+abs(sobel_y[y4][x4]),0,4095);
            gradient_img[y4][x4] = temp;
        }
    }
    //singleChannel2BMP(gradient_img,w,h,gradient_map);


    for(uint16 p=0;p <h;p++){
        free(sobel_y[p]);
    }
    free(sobel_y);

    for(uint16 q=0;q <h;q++){
        free(sobel_x[q]);
    }
    free(sobel_x);

    return;
}
void core_bpc(uint16** input,const raw_info rawInfo,bpc_para bpcPara){


    //judge hot pixel or dead pixel
    uint16 height = rawInfo.u16ImgHeight;
    uint16 width = rawInfo.u16ImgWidth;
    //uint8 BP = rawInfo.u8BayerPattern;
    uint16 bpc_thresh_D = bpcPara.bpc_thr_d;
    uint16 bpc_thresh_C = bpcPara.bpc_thr_c;
    uint16 bpc_slope = bpcPara.bpc_slope;
    uint16 bpc_egde_threshold = bpcPara.bpc_edge_threshold;
    //uint8 bpc_blend = bpcPara.bpc_blend;
    uint8 alp = 0;
    uint16 surr_pixels[8]={0};
    uint16 sort_surr_pixels[8];
    uint16 surr_max = 0;
    uint16 surr_max2 = 0;
    uint16 surr_min =0;
    uint16 surr_min2 =0;
    uint16 replace_pixel1 = 0;


    uint16** gradient_img = (uint16**)malloc(height*sizeof(uint16*));
    for(uint16 v1=0;v1 <height;v1++){
        gradient_img[v1] = (uint16*) malloc(width*sizeof(uint16));
    }
    memset(&gradient_img[0][0],0,sizeof(gradient_img));
    generate_gradient(input,gradient_img,width,height);
    for(uint16 y=2;y<height-2;y++){
        for(uint16 x=2;x<width-2;x++){
                surr_pixels[0] = input[y-2][x-2];surr_pixels[1] = input[y-2][x];
                surr_pixels[2] = input[y-2][x+2];surr_pixels[3] = input[y][x-2];
                surr_pixels[4] = input[y][x+2];surr_pixels[4] = input[y+2][x-2];
                surr_pixels[6] = input[y+2][x];surr_pixels[7] = input[y+2][x+2];
                uint16 temp = input[y][x];
                memcpy(sort_surr_pixels,surr_pixels,sizeof(surr_pixels));
                bubble_sort(sort_surr_pixels,8);
                surr_max = sort_surr_pixels[7];
                surr_max2 = sort_surr_pixels[6];
                surr_min = sort_surr_pixels[0];
                surr_min2 = sort_surr_pixels[1];

                //printf("LINE-%d,edge_flag:%d \n",__LINE__,edge_flag);

                uint16 avg_grad = (gradient_img[y+1][x]+gradient_img[y-1][x]+gradient_img[y][x+1]+gradient_img[y][x-1])>>2;
                if(avg_grad >bpc_egde_threshold){
                //may try treating min gradient direction as edge direction

                    //replace_pixel1 = input[y][x];
                }
                else{
                    //hot pixel
                    if(temp>surr_max && (temp-surr_max2)>bpc_thresh_D){
                        if((surr_max-surr_max2)<bpc_thresh_C){
                            replace_pixel1 = surr_max;
                            alp = 1 + (bpc_slope>>4)*(temp -surr_max - bpc_thresh_D);


                        }
                        else{
                            replace_pixel1 = surr_max2;
                            alp = 1 + (bpc_slope>>4)*(temp -surr_max2 - bpc_thresh_D);
                        }
                    }
                    //dead pixel
                    else if(temp<surr_min && ((surr_min2-temp) > bpc_thresh_D)){
                        if((surr_min2-surr_min)<bpc_thresh_C){
                            replace_pixel1 = surr_min;
                            alp = 1 + (bpc_slope>>4)*(surr_min -temp - bpc_thresh_D);
                        }
                        else{
                            replace_pixel1 = surr_min2;
                            alp = 1 + (bpc_slope>>4)*(surr_min2 -temp - bpc_thresh_D);
                        }
                    }
                        //normal pixel
                    else{
                        replace_pixel1 = input[y][x];
                        alp = 0;
                    }
                    //printf("LINE-%d,replace_pixel1:%d \n",__LINE__,replace_pixel1);
                    //printf("LINE-%d,alp:%d \n",__LINE__,alp);
                    input[y][x] =(input[y][x]*(256-alp) + alp *replace_pixel1)>>8;
                }

        }
    }

    for(uint16 i=0;i <height;i++){
        free(gradient_img[i]);
    }
    free(gradient_img);

    return;
}


int main(int argc,char**argv){

//    static uint16 gamma_table[] = {
//        #include "gamma_table.h"
//    };

    const raw_info raw_info = {0,0,1920,1080,12,BPRG};
    bpc_para isp_bpc_para ={1000,500,32,2000,128};
    uint16 raw_width = raw_info.u16ImgWidth;
    uint16 raw_height = raw_info.u16ImgHeight;
    char *raw_file = "D:\\all_isp\\test_img\\iso2W_1920x1080_12bits_RGGB_Linear.raw";
    char *raw_file_100 = "D:\\all_isp\\test_img\\iso100_1920x1080_12bits_RGGB_Linear.raw";
    char *before_bpc = "D:\\leetcode_project\\before_bpc_img.bmp";
    char *after_bpc = "D:\\leetcode_project\\after_bpc_img.bmp";

    uint16* BayerImg = (uint16*)malloc(raw_width * raw_height * sizeof(uint16));
    if( NULL == BayerImg ){
        printf("BayerImg malloc fail!!!\n");
    }
    uint16** m_BayerImg = (uint16**)malloc(raw_height*sizeof(uint16*));
    for(uint16 v=0;v < raw_height;v++){
        m_BayerImg[v] = (uint16*) malloc(raw_width*sizeof(uint16));
    }


    read_BayerImg(raw_file,raw_height,raw_width,BayerImg);
    //print_raw_to_txt(BayerImg,1920,1080,save_raw);
    reshape(BayerImg,m_BayerImg,raw_width,raw_height);
    //edge_detector(m_BayerImg,raw_width,raw_height);

    //singleChannel2BMP(m_BayerImg,raw_width,raw_height,raw_file_100);
    //singleChannel2BMP(m_BayerImg,raw_width,raw_height,before_bpc);
    core_bpc(m_BayerImg,raw_info,isp_bpc_para);
    singleChannel2BMP(m_BayerImg,raw_width,raw_height,after_bpc);


    for(uint16 p=0; p <raw_height;p++){
        free(m_BayerImg[p]);
    }
    free(m_BayerImg);


    free(BayerImg);
    return 0;

}





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */