#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <SPIFFS.h>
#include <FS.h>
#include "SD.h"

#define RX_BCLK 17
#define RX_WS 5
#define RX_DOUT I2S_PIN_NO_CHANGE
#define RX_DIN 39

#define TX_BCLK 26
#define TX_WS 25
#define TX_DOUT 32
#define TX_DIN I2S_PIN_NO_CHANGE

static int sample_rate = 44100;

void init_i2s() {
    /* TX: I2S_NUM_0 */
    i2s_config_t i2s_config_tx;
    i2s_config_tx.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    i2s_config_tx.sample_rate = sample_rate;
    i2s_config_tx.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    i2s_config_tx.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;   // 2-channels
    i2s_config_tx.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
    i2s_config_tx.dma_buf_count = 32;                            // number of buffers, 128 max.
    i2s_config_tx.dma_buf_len = 32 * 2;                          // size of each buffer
    i2s_config_tx.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;        // Interrupt level 1
    
    i2s_pin_config_t pin_config_tx;
    pin_config_tx.bck_io_num = TX_BCLK;
    pin_config_tx.ws_io_num = TX_WS;
    pin_config_tx.data_out_num = TX_DOUT;
    pin_config_tx.data_in_num = I2S_PIN_NO_CHANGE; // GPIO_NUM_23;

     i2s_driver_install(I2S_NUM_0, &i2s_config_tx, 0, NULL);
     i2s_set_pin(I2S_NUM_0, &pin_config_tx);

    /* RX: I2S_NUM_0 */
    i2s_config_t i2s_config_rx;
    i2s_config_rx.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX); // Only TX
    i2s_config_rx.sample_rate = sample_rate;
    i2s_config_rx.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;    // Only 8-bit DAC support
    i2s_config_rx.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;   // 2-channels
    i2s_config_rx.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
    i2s_config_rx.dma_buf_count = 32;                            // number of buffers, 128 max.
    i2s_config_rx.dma_buf_len = 32 * 3;                          // size of each buffer
    i2s_config_rx.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;

    i2s_pin_config_t pin_config_rx;
    pin_config_rx.bck_io_num = RX_BCLK;
    pin_config_rx.ws_io_num = RX_WS;
    pin_config_rx.data_out_num = I2S_PIN_NO_CHANGE;
    pin_config_rx.data_in_num = RX_DIN;

    i2s_driver_install(I2S_NUM_1, &i2s_config_rx, 0, NULL);
    i2s_set_pin(I2S_NUM_1, &pin_config_rx);
}

void read_and_print_sample() {

        uint32_t cnt = 0;
        uint32_t buffer;
        uint32_t buffer_out = 0;
        i2s_start(I2S_NUM_0);
        i2s_start(I2S_NUM_1);
    
        while(1){
            cnt++;
            buffer = 0;
            int bytes_popped = i2s_pop_sample(I2S_NUM_1, (char*)&buffer, portMAX_DELAY);
        
            buffer_out = buffer << 5;
    
            if (buffer_out == 0) {
                //For debugging, if out is zero
                Serial.printf("%d -> %x\n", cnt, (int)buffer_out);
                delay(50);
            }
            else {
                //Just playback for now
                i2s_push_sample(I2S_NUM_0, (char*)&buffer_out, portMAX_DELAY);
            }
        }
    }
    


void recording(){
    fs::FS fs = SD;
    
    
    
        File file = fs.open("/wishList.wav", FILE_WRITE);
        if (!file) {
            Serial.print( "Failed to open file for writing");
            return;
        }
        int cnt = 0;
        uint64_t buffer = 0;
        char buf[512];
    
        unsigned elapsedTime = millis();
    
        i2s_start(I2S_NUM_1);
        while (1)
        {
            char *buf_ptr_read = buf;
            char *buf_ptr_write = buf;
    
            // read whole block of samples
            int bytes_read = 0;
            while(bytes_read == 0) {
                    bytes_read = i2s_pop_sample(I2S_NUM_1, (char*)buf, portMAX_DELAY);
            }
    
            uint32_t samples_read = bytes_read / 2 / (I2S_BITS_PER_SAMPLE_32BIT / 8);
    
            //const char samp64[8] = {};
            for (int i = 0; i < samples_read; i++)
            {
                buf_ptr_write[0] = buf_ptr_read[2]; // mid
                buf_ptr_write[1] = buf_ptr_read[3]; // high
    
                buf_ptr_write += 1 * (I2S_BITS_PER_SAMPLE_16BIT / 8);
                buf_ptr_read += 2 * (I2S_BITS_PER_SAMPLE_32BIT / 8);
            }
    
            size_t readable_bytes = samples_read * (I2S_BITS_PER_SAMPLE_16BIT / 8);
    
            //  2 * mono
            //i2s_write_bytes(I2S_NUM_0, (const char*)buf, readable_bytes, portMAX_DELAY);
            //i2s_push_sample(I2S_NUM_0, (const char*)buf, portMAX_DELAY);
            //fwrite((const char*)buf, sizeof(char), readable_bytes, f);
            file.write((uint8_t*)buf, readable_bytes);
    
            if (millis() - elapsedTime > 10000) {
                    printf("Recording done\n");
                    elapsedTime = millis();
    
                    break;
            }
        }
    
        //fclose(f);
        file.close();

     
    
        Serial.print("selesai");
        //return;
        //vTaskDelete(NULL);
        Serial.print("mainkan");
        ESP_LOGI(TAG2, "Opening file for playing");

        ESP_LOGI(TAG2, "Reading file");
         file = fs.open("/wishList.wav", FILE_READ);
    
        if (!file) {
            ESP_LOGE(TAG2, "Failed to open file for reading");
            return;
        }
    
    //	i2s_set_sample_rates(I2S_NUM_0, 11000); //set sample rates
    
        uint8_t buf_bytes_per_sample = (I2S_BITS_PER_SAMPLE_16BIT / 8);
        int buffer_len = -1;
        int buffer_index = 0;
        char buffer_read[512];
        int readed = 0;
    
        i2s_start(I2S_NUM_0);
    
        while(1) {
    
            //readed = fread((char*)buffer_read, sizeof(char), sizeof(buffer_read), f);
            readed = file.read((uint8_t*)buffer_read, sizeof(buffer_read));
    
            if (readed > 0) {
    
                buffer_len = readed / sizeof(buffer_read[0]);
                buffer_index = 0;
    
    //			delay(1);
    
                char *ptr_l = buffer_read;
                char *ptr_r = ptr_l;
                uint8_t stride = buf_bytes_per_sample;
                uint32_t num_samples = buffer_len / buf_bytes_per_sample / 1;
                //printf("Read size: %d, num samples: %d\r\n", readed, num_samples);
    
                int bytes_pushed = 0;
                for (int i = 0; i < num_samples; i++) {
    
                    const char samp32[4] = {ptr_l[0], ptr_l[1], ptr_r[0], ptr_r[1]};
    
                    bytes_pushed = i2s_push_sample(I2S_NUM_0, (const char*) &samp32, portMAX_DELAY);
    
                    // DMA buffer full - retry
                    if (bytes_pushed == 0) {
                        i--;
                    } else {
                        ptr_r += stride;
                        ptr_l += stride;
                    }
                }
    
            } else {
                printf("Stop playing\n");
    
                i2s_stop(I2S_NUM_0);
                break;
            }
        }
    
        file.close();
        return;
        vTaskDelete(NULL);

            
    }


    void tryingToplay(){
        ESP_LOGI(TAG2, "Opening file for playing");
        fs::FS fs = SD;
            // Open file for reading
            printf( "Reading file\n");
            File file = fs.open("/indonesia.wav", FILE_READ);
        
            if (!file) {
                printf( "Failed to open file for reading/n");
                return;
            }
        
        //	i2s_set_sample_rates(I2S_NUM_0, 11000); //set sample rates
        
            uint8_t buf_bytes_per_sample = (I2S_BITS_PER_SAMPLE_16BIT / 8);
            int buffer_len = -1;
            int buffer_index = 0;
            char buffer_read[512];
            int readed = 0;
        
            i2s_start(I2S_NUM_0);
        
            while(1) {
        
                //readed = fread((char*)buffer_read, sizeof(char), sizeof(buffer_read), f);
                readed = file.read((uint8_t*)buffer_read, sizeof(buffer_read));
        
                if (readed > 0) {
        
                    buffer_len = readed / sizeof(buffer_read[0]);
                    buffer_index = 0;
        
        //			delay(1);
        
                    char *ptr_l = buffer_read;
                    char *ptr_r = ptr_l;
                    uint8_t stride = buf_bytes_per_sample;
                    uint32_t num_samples = buffer_len / buf_bytes_per_sample / 1;
                    //printf("Read size: %d, num samples: %d\r\n", readed, num_samples);
        
                    int bytes_pushed = 0;
                    for (int i = 0; i < num_samples; i++) {
        
                        const char samp32[4] = {ptr_l[0], ptr_l[1], ptr_r[0], ptr_r[1]};
        
                        bytes_pushed = i2s_push_sample(I2S_NUM_0, (const char*) &samp32, portMAX_DELAY);
        
                        // DMA buffer full - retry
                        if (bytes_pushed == 0) {
                            i--;
                        } else {
                            ptr_r += stride;
                            ptr_l += stride;
                        }
                    }
        
                } else {
                    printf("Stop playing\n");
        
                    i2s_stop(I2S_NUM_0);
                    break;
                }
            }
        
            file.close();
            return;
            vTaskDelete(NULL);
    }

    void serialPlotter() {
        uint32_t buffer;
        uint32_t buffer_out;
    
        int bytes_popped = i2s_pop_sample(I2S_NUM_1, (char*)&buffer, portMAX_DELAY);
        buffer_out = buffer << 5;
    
        Serial.printf("%u\n", buffer);
    
        // i2s_push_sample(I2S_NUM_0, (char*)&buffer_out, portMAX_DELAY);
    
        // delay(50);
    } 