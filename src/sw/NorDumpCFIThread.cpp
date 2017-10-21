#include "NorDumpCFIThread.h"
#include "Flasher.h"

NorDumpCFIThread::NorDumpCFIThread(const QString& _dumpFile, CommonArgs _commonArgs, NorArgs *_norArgs) :
    NorCommonThread(_commonArgs, *_norArgs), dumpFile(_dumpFile), norargs(_norArgs)
{
}

void NorDumpCFIThread::Operation()
{
    /* Fuck you, progress ! */
    progressRangeChanged(0, 1);
    progressChanged(0);



    ::NOR_Configure(commonArgs, norArgs);

    ::NOR_Reset();

    QByteArray cfiBuf(0x102, 0x00);
    int res = ::NOR_ReadCFI(cfiBuf.data(), cfiBuf.length());
    if(res < 0)
        statusTextAdded(tr("LibUSB Error: %1").arg(res));

    if(dumpFile != "") {

        QFile fp(dumpFile);
        if (!fp.open(QIODevice::WriteOnly)) {
            statusTextAdded(tr("Failed to open file for writing"));
            return;
        }
        fp.write(cfiBuf);
        fp.close();

    }
    Decode(cfiBuf);

    progressChanged(1);
}

void NorDumpCFIThread::Decode(const QByteArray& cfi)
{
    const char* cfip = cfi.data();

    float vcc_supply_min, vcc_supply_max, vpp_supply_min, vpp_supply_max;
    uint32_t timeout_per_word_program;
    uint32_t timeout_minimum_size_write_buffer;
    uint32_t timeout_per_block_erase;
    uint32_t timeout_fullchip_erase;
    uint32_t maximum_timeout_per_word_program;
    uint32_t maximum_timeout_write_buffer;
    uint32_t maximum_timeout_per_block_erase;
    uint32_t maximum_timeout_fullchip_erase;
    uint32_t device_size;
    uint32_t maximum_number_of_bytes;
    uint8_t flash_device_interface;
    uint32_t number_of_erase_block_regions;
    uint32_t region_erase_block[3];
    uint32_t region_block_size[3];

    if (cfip[0] != 'Q' || cfi[1] != 'R' || cfi[2] != 'Y') {
        statusTextAdded(tr("CFI is invalid (does not begin with \"QRY\")"));
    } else {
        statusTextAdded(tr("Decoding CFI"));

        vcc_supply_min = (float)((cfip[0x0B] & 0xF0) / 16 )+ (float)(cfi[0x0B] & 0x0F)/10;
        vcc_supply_max = (float)((cfip[0x0C] & 0xF0) / 16 )+ (float)(cfi[0x0C] & 0x0F)/10;
        vpp_supply_min = (float)((cfip[0x0D] & 0xF0) / 16 )+ (float)(cfi[0x0D] & 0x0F)/10;
        vpp_supply_max = (float)((cfip[0x0E] & 0xF0) / 16 )+ (float)(cfi[0x0E] & 0x0F)/10;

        timeout_per_word_program = 1<<cfip[0x0F];
        timeout_minimum_size_write_buffer = cfip[0x10]?1<<cfip[0x10]:0;
        timeout_per_block_erase = 1<<cfip[0x11];
        timeout_fullchip_erase = cfip[0x12]?1<<cfip[0x12]:0;
        maximum_timeout_per_word_program = timeout_per_word_program * 1<<cfip[0x13];
        maximum_timeout_write_buffer = timeout_minimum_size_write_buffer * 1<<cfip[0x14];
        maximum_timeout_per_block_erase = timeout_per_block_erase * 1<<cfip[0x15];
        maximum_timeout_fullchip_erase = timeout_fullchip_erase * 1<<cfip[0x16];

        device_size = 1<<cfip[0x17]; norargs->deviceSize = device_size;
        maximum_number_of_bytes = 1<<cfip[0x1A]; norargs->bufferedMax = maximum_number_of_bytes;
        flash_device_interface = cfip[0x18];
        number_of_erase_block_regions = cfip[0x1C]; norargs->regions.count = number_of_erase_block_regions;

        statusTextAdded(tr(" - VCC = %1V - %2V").arg(vcc_supply_min).arg(vcc_supply_max));
        if (vpp_supply_min > 0.0)
            statusTextAdded(tr(" - VPP = %1V - %2V").arg(vpp_supply_min).arg(vpp_supply_max));
        else
            statusTextAdded(tr(" - VPP = NA"));

        statusTextAdded(tr(" - Timeout per word program = %1us").arg(timeout_per_word_program));
        norargs->timings.tWHWH1_Word = timeout_per_word_program;

        if (timeout_minimum_size_write_buffer)
            statusTextAdded(tr(" - Timeout for minimum buffer write = %1us").arg(timeout_minimum_size_write_buffer));
        else
            statusTextAdded(tr(" - Timeout for minimum buffer write = NA"));
        norargs->timings.tWHWH1_BufWord = timeout_minimum_size_write_buffer;


        statusTextAdded(tr(" - Timeout per individual block erase = %1ms").arg(timeout_per_block_erase));

        if (timeout_fullchip_erase)
            statusTextAdded(tr(" - Timeout for full chip erase = %1ms").arg(timeout_fullchip_erase));
        else
            statusTextAdded(tr(" - Timeout for full chip erase = NA"));

        statusTextAdded(tr(" - Maximum timeout per word program = %1us").arg(maximum_timeout_per_word_program));

        if (maximum_timeout_write_buffer)
            statusTextAdded(tr(" - Maximum timeout for write buffer = %1us").arg(maximum_timeout_write_buffer));
        else
            statusTextAdded(tr(" - Maximum timeout for write buffer = NA"));

        statusTextAdded(tr(" - Maximum timeout per block erase = %1s").arg(maximum_timeout_per_block_erase/1000));

        if (maximum_timeout_fullchip_erase)
            statusTextAdded(tr(" - Maximum timeout for full chip erase %1ms").arg(maximum_timeout_fullchip_erase));
        else
            statusTextAdded(tr(" - Maximum timeout for full chip erase = NA"));

        statusTextAdded(tr(" - Device size = %1MiB (%2)").arg(device_size/1024/1024).arg(device_size));

        statusTextAdded(tr(" - Maximum number of bytes for buffered write = %1").arg(maximum_number_of_bytes));

        statusTextAdded(tr(" - Flash device interface code description = %1").arg(
                            flash_device_interface == 0 ? tr("8bits only") :
                                                          (flash_device_interface == 1 ?
                                                               tr("16bits only") :
                                                               tr("8bits/16bits"))));

        statusTextAdded(tr(" - Number of erase block regions = %1").arg(number_of_erase_block_regions));

        for(uint8_t r=0; r < number_of_erase_block_regions; r++) {
            statusTextAdded(tr(" - Region %1").arg(r+1));

            region_erase_block[r] = (uint32_t)cfip[0x1D+(r<<2)]+1;
            region_block_size[r] = ( (cfip[0x20+(r<<2)]<<8) + (uint32_t)cfip[0x1f + (r<<2)]) << 8;
            statusTextAdded(tr("    - Region %1 number of erase blocks = %2").arg(r+1).arg(region_erase_block[r]));

            norargs->regions.region[r].blockCount = region_erase_block[r];
            norargs->regions.region[r].blockSize = region_block_size[r];

            if (region_block_size[r] > 1024-1) {
                if (region_block_size[r] > 1024*1024-1)
                    statusTextAdded(tr("    - Region %1 block size = %2Mbyte").arg(r+1).arg(region_block_size[r]/1024/1024));
                else
                    statusTextAdded(tr("    - Region %1 block size = %2Kbyte").arg(r+1).arg(region_block_size[r]/1024));
            } else {
                statusTextAdded(tr("    - Region %1 block size = %2 bytes").arg(r+1).arg(region_block_size[r]));
            }
        }
    }
}
