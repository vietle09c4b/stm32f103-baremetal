/* Rte.h - phần RTE dành cho OS/EcuM: khởi động SWC và thân task chứa runnable. */
#ifndef RTE_H
#define RTE_H

void Rte_Start(void);         /* gọi runnable khởi tạo của SWC (InitEvent) */
void Rte_Task_10ms(void);

#endif /* RTE_H */
