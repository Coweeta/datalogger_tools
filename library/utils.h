#include <SdFat.h>

namespace coweeta {

void print_root_directory(const SdFat &sd_card);
const char*  build_filename(uint16_t file_num);

} // namespace coweeta
