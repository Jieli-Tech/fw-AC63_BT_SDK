# 总的 Makefile，用于调用目录下各个子工程对应的 Makefile
# 注意： Linux 下编译方式：
# 1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
# 2. 下载后，解压到 /opt/jieli 目录下，保证
#   /opt/jieli/common/bin/clang 存在（注意目录层次）
# 3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#   可以通过 ulimit -n 8096 来设置一个较大的值
# 支持的目标
# make ac638n_spp_and_le
# make ac632n_spp_and_le
# make ac631n_spp_and_le
# make ac636n_spp_and_le
# make ac637n_spp_and_le
# make ac635n_spp_and_le
# make ac638n_hid
# make ac632n_hid
# make ac631n_hid
# make ac636n_hid
# make ac637n_hid
# make ac635n_hid
# make ac638n_mesh
# make ac632n_mesh
# make ac631n_mesh
# make ac636n_mesh
# make ac637n_mesh
# make ac635n_mesh

.PHONY: all clean ac638n_spp_and_le ac632n_spp_and_le ac631n_spp_and_le ac636n_spp_and_le ac637n_spp_and_le ac635n_spp_and_le ac638n_hid ac632n_hid ac631n_hid ac636n_hid ac637n_hid ac635n_hid ac638n_mesh ac632n_mesh ac631n_mesh ac636n_mesh ac637n_mesh ac635n_mesh clean_ac638n_spp_and_le clean_ac632n_spp_and_le clean_ac631n_spp_and_le clean_ac636n_spp_and_le clean_ac637n_spp_and_le clean_ac635n_spp_and_le clean_ac638n_hid clean_ac632n_hid clean_ac631n_hid clean_ac636n_hid clean_ac637n_hid clean_ac635n_hid clean_ac638n_mesh clean_ac632n_mesh clean_ac631n_mesh clean_ac636n_mesh clean_ac637n_mesh clean_ac635n_mesh

all: ac638n_spp_and_le ac632n_spp_and_le ac631n_spp_and_le ac636n_spp_and_le ac637n_spp_and_le ac635n_spp_and_le ac638n_hid ac632n_hid ac631n_hid ac636n_hid ac637n_hid ac635n_hid ac638n_mesh ac632n_mesh ac631n_mesh ac636n_mesh ac637n_mesh ac635n_mesh
	@echo +ALL DONE

clean: clean_ac638n_spp_and_le clean_ac632n_spp_and_le clean_ac631n_spp_and_le clean_ac636n_spp_and_le clean_ac637n_spp_and_le clean_ac635n_spp_and_le clean_ac638n_hid clean_ac632n_hid clean_ac631n_hid clean_ac636n_hid clean_ac637n_hid clean_ac635n_hid clean_ac638n_mesh clean_ac632n_mesh clean_ac631n_mesh clean_ac636n_mesh clean_ac637n_mesh clean_ac635n_mesh
	@echo +CLEAN DONE

ac638n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/br34 -f Makefile

clean_ac638n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/br34 -f Makefile clean

ac632n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/bd19 -f Makefile

clean_ac632n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/bd19 -f Makefile clean

ac631n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/bd29 -f Makefile

clean_ac631n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/bd29 -f Makefile clean

ac636n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/br25 -f Makefile

clean_ac636n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/br25 -f Makefile clean

ac637n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/br30 -f Makefile

clean_ac637n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/br30 -f Makefile clean

ac635n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/br23 -f Makefile

clean_ac635n_spp_and_le:
	$(MAKE) -C apps/spp_and_le/board/br23 -f Makefile clean

ac638n_hid:
	$(MAKE) -C apps/hid/board/br34 -f Makefile

clean_ac638n_hid:
	$(MAKE) -C apps/hid/board/br34 -f Makefile clean

ac632n_hid:
	$(MAKE) -C apps/hid/board/bd19 -f Makefile

clean_ac632n_hid:
	$(MAKE) -C apps/hid/board/bd19 -f Makefile clean

ac631n_hid:
	$(MAKE) -C apps/hid/board/bd29 -f Makefile

clean_ac631n_hid:
	$(MAKE) -C apps/hid/board/bd29 -f Makefile clean

ac636n_hid:
	$(MAKE) -C apps/hid/board/br25 -f Makefile

clean_ac636n_hid:
	$(MAKE) -C apps/hid/board/br25 -f Makefile clean

ac637n_hid:
	$(MAKE) -C apps/hid/board/br30 -f Makefile

clean_ac637n_hid:
	$(MAKE) -C apps/hid/board/br30 -f Makefile clean

ac635n_hid:
	$(MAKE) -C apps/hid/board/br23 -f Makefile

clean_ac635n_hid:
	$(MAKE) -C apps/hid/board/br23 -f Makefile clean

ac638n_mesh:
	$(MAKE) -C apps/mesh/board/br34 -f Makefile

clean_ac638n_mesh:
	$(MAKE) -C apps/mesh/board/br34 -f Makefile clean

ac632n_mesh:
	$(MAKE) -C apps/mesh/board/bd19 -f Makefile

clean_ac632n_mesh:
	$(MAKE) -C apps/mesh/board/bd19 -f Makefile clean

ac631n_mesh:
	$(MAKE) -C apps/mesh/board/bd29 -f Makefile

clean_ac631n_mesh:
	$(MAKE) -C apps/mesh/board/bd29 -f Makefile clean

ac636n_mesh:
	$(MAKE) -C apps/mesh/board/br25 -f Makefile

clean_ac636n_mesh:
	$(MAKE) -C apps/mesh/board/br25 -f Makefile clean

ac637n_mesh:
	$(MAKE) -C apps/mesh/board/br30 -f Makefile

clean_ac637n_mesh:
	$(MAKE) -C apps/mesh/board/br30 -f Makefile clean

ac635n_mesh:
	$(MAKE) -C apps/mesh/board/br23 -f Makefile

clean_ac635n_mesh:
	$(MAKE) -C apps/mesh/board/br23 -f Makefile clean
