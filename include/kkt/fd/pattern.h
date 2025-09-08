#ifndef __PATTERN_H__
#define __PATTERN_H__

#include <stdint.h>

// �᢮������� ������, ������� 蠡������
void kkt_free_patterns(void);
// ��१���㦠�� 蠡���� ��� 㪠������� ���
void kkt_reload_patterns(uint64_t inn);
// ��室�� 蠡��� �� ⨯� ���㬥�� � �������
const uint8_t *kkt_find_pattern(uint8_t docType, uint8_t index, size_t *size);


#endif
