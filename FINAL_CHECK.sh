#!/bin/bash
echo "=========================================="
echo "  项目最终检查"
echo "=========================================="
echo ""

echo "✅ 检查源代码文件..."
for file in filesystem.h filesystem.cpp shell.h shell.cpp main.cpp; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file (缺失)"
    fi
done
echo ""

echo "✅ 检查文档文件..."
for file in START_HERE.md QUICKSTART.md EXAMPLES.md README.md ARCHITECTURE.md PROJECT_SUMMARY.md INDEX.md PROJECT_COMPLETION_REPORT.md FILES_DESCRIPTION.md; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file (缺失)"
    fi
done
echo ""

echo "✅ 检查构建文件..."
for file in Makefile .gitignore test_demo.sh; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file (缺失)"
    fi
done
echo ""

echo "✅ 检查编译状态..."
if [ -f "myfs" ]; then
    echo "  ✓ 可执行文件已生成"
    echo "  大小: $(ls -lh myfs | awk '{print $5}')"
else
    echo "  ✗ 可执行文件未生成"
fi
echo ""

echo "✅ 统计信息..."
echo "  源代码文件: $(ls -1 *.cpp *.h 2>/dev/null | wc -l)"
echo "  文档文件: $(ls -1 *.md 2>/dev/null | wc -l)"
echo "  代码总行数: $(cat *.cpp *.h 2>/dev/null | wc -l)"
echo "  文档总行数: $(cat *.md 2>/dev/null | wc -l)"
echo ""

echo "=========================================="
echo "  检查完成！"
echo "=========================================="
echo ""
echo "🚀 准备就绪！可以开始使用了："
echo "   ./myfs"
echo ""
