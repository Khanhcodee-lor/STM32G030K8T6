#ifndef ANALOG_IO_SERVICE_H
#define ANALOG_IO_SERVICE_H

int analog_io_service_apply_safe_output_blocking(void);
void analog_io_service_init(void);
void analog_io_service_run_once(void);

#endif /* ANALOG_IO_SERVICE_H */
