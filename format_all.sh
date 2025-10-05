#!/bin/bash
# 格式化所有 C++ 文件，确保行宽不超过 120 字符

echo "正在格式化所有 C++ 文件..."
echo "使用 .clang-format 配置（行宽限制: 120）"
echo ""

# 查找并格式化所有 C++ 文件
find . -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o -name "*.h" -o -name "*.inl" \) \
    -not -path "*/build/*" \
    -not -path "*/third_party/*" \
    -not -path "*/.git/*" \
    -print0 | while IFS= read -r -d '' file; do
    echo "格式化: $file"
    clang-format -i "$file"
done

echo ""
echo "✅ 格式化完成！"
echo ""
echo "检查是否还有超过 120 字符的行..."
find . -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o -name "*.h" -o -name "*.inl" \) \
    -not -path "*/build/*" \
    -not -path "*/third_party/*" \
    -not -path "*/.git/*" \
    -exec awk 'length > 120 {print FILENAME":"NR":"length" chars"}' {} \; | head -20

echo ""
echo "如果上面没有输出，说明所有文件都符合 120 字符限制！"
