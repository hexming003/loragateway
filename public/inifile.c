#include<string.h>  
#include <stdio.h>

static int filter_value(char *value)
{
    int i,j;
    char filter_c[] = {'\r','\n'};
    
    for(i = 0; i < strlen(value); i++)
    {
        for(j = 0; j < sizeof(filter_c); j++)
        {        
            if(value[i] == filter_c[j])
            {
                value[i] = 0;
            }
        }
    }
    return 0;
}
char* GetIniSectionItem(char* filename,char* item)  
//char* get_avp_val(char* filename,char* section,char* item)  
{  
    char *value,c;  
    static char linebuf[1024];  //return value, static variable, not multithread safe    
    FILE *inifp;      
    
    if (!filename || !item)
    {
        printf("1111222222\n");
        return NULL;
    }    
    
    /*strcpy(Ite,"[");strcat(Ite,Item);strcat(Ite,"]");*/  
    if((inifp=fopen(filename,"rb"))==NULL)  
    {  
                   printf("1111\n");
        return NULL;  
    }  
    
    //printf("filename = %s,Item = %s /n",filename,item);

    while(fgets(linebuf,1024,inifp) != NULL )  
    {    
                
        //printf("linebuf %s\n",linebuf);
        if(strstr(linebuf,item))  
        {  
            if((value = strchr(linebuf,'=')))
            {  
                value++;  
                /*printf("value = %s /n",value);*/  
                fclose(inifp);  
                if(*value=='\n')  
                    {
                    
                    printf("1122\n");
                    return (char *)NULL;
                    }
                
                //printf("1124 %s\n", value);
                while(*value==' ')
                {
                    value++; 
                }
                
                //printf("1122 %s\n", value);
                filter_value(value);
                
                return value;  
            }  
        }     
    }  
    fclose(inifp);  
    
    printf("1122333\n");
    return (char*)NULL;  
}  

#if 0
int main()
{
    char* aaa;
    aaa = GetIniSectionItem("/home/lxm/tmp/test.ini","Visio Database Samples","Driver32");
    if (aaa)
    {
        printf("aaa= %s\n",aaa);
    }
    else
    {
        printf("aaa is null\n");
    }
    return 0;        
}

#endif

