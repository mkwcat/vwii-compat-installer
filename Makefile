.PHONY: clean

all:
	@$(MAKE) -C arm_user
	@$(MAKE) -C wupserver
	@$(MAKE) -C arm_kernel
	@$(MAKE) -C src

clean:
	@$(MAKE) -C arm_user clean
	@$(MAKE) -C wupserver clean
	@$(MAKE) -C arm_kernel clean
	@$(MAKE) -C src clean
