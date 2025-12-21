#!/bin/bash
echo "======================================"
echo "  项目统计报告"
echo "======================================"
echo ""

echo "📁 文件统计："
echo "  源代码文件: $(find . -name "*.cpp" -o -name "*.h" | wc -l)"
echo "  文档文件: $(find . -name "*.md" | wc -l)"
echo "  总文件数: $(ls -1 | wc -l)"
echo ""

echo "📊 代码统计："
echo "  .h 文件行数: $(cat *.h 2>/dev/null | wc -l)"
echo "  .cpp 文件行数: $(cat *.cpp 2>/dev/null | wc -l)"
echo "  总代码行数: $(cat *.h *.cpp 2>/dev/null | wc -l)"
echo ""

echo "📖 文档统计："
echo "  文档总行数: $(cat *.md 2>/dev/null | wc -l)"
echo ""

echo "💾 磁盘使用："
echo "  源代码大小: $(du -sh *.cpp *.h 2>/dev/null | awk '{sum+=$1} END {print sum "K"}')"
echo "  可执行文件: $(ls -lh myfs 2>/dev/null | awk '{print $5}')"
echo "  虚拟磁盘: $(ls -lh disk.bin 2>/dev/null | awk '{print $5}')"
echo ""

echo "✅ 编译状态："
if [ -f "myfs" ]; then
    echo "  可执行文件: 存在 ✓"
else
    echo "  可执行文件: 不存在 ✗"
fi

if [ -f "disk.bin" ]; then
    echo "  虚拟磁盘: 存在 ✓"
else
    echo "  虚拟磁盘: 未创建"
fi
echo ""

echo "======================================"
echo "  统计完成"
echo "======================================"
