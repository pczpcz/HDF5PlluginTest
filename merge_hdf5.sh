#!/bin/bash
# HDF5文件合并脚本
# 自动合并当前目录下的HDF5拆分文件

# 自动查找拆分文件
PART1=$(ls PBG08621_pass_6c7986d6_167483a9_0_part1.hdf5 2>/dev/null)
PART2=$(ls PBG08621_pass_6c7986d6_167483a9_0_part2.hdf5 2>/dev/null)
PART3=$(ls PBG08621_pass_6c7986d6_167483a9_0_part3.hdf5 2>/dev/null)
PART4=$(ls PBG08621_pass_6c7986d6_167483a9_0_part4.hdf5 2>/dev/null)
PART5=$(ls PBG08621_pass_6c7986d6_167483a9_0_part5.hdf5 2>/dev/null)
OUTPUT_FILE="PBG08621_pass_6c7986d6_167483a9_0.hdf5"



# 合并文件
echo "正在合并文件..."
cat "$PART1" "$PART2" "$PART3" "$PART4" "$PART5" > "$OUTPUT_FILE"
# 检查合并结�?
if [ $? -eq 0 ]; then
    echo "合并完成! 输出文件: $OUTPUT_FILE"
    echo "文件大小: $(du -h "$OUTPUT_FILE" | cut -f1)"
    
    echo "�?合并成功!"
else
    echo "�?合并失败!"
    exit 1
fi