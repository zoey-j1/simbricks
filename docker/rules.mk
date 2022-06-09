# Copyright 2022 Max Planck Institute for Software Systems, and
# National University of Singapore
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

include mk/subdir_pre.mk

docker-images:
	docker build -t simbricks/simbricks-build:latest \
		-f docker/Dockerfile.buildenv docker
	docker build -t simbricks/simbricks:latest \
		-f docker/Dockerfile .
	docker build -t simbricks/simbricks-runenv:latest \
		-f docker/Dockerfile.runenv docker
	docker build -t simbricks/simbricks-min:latest \
		-f docker/Dockerfile.min docker
	docker build -t simbricks/simbricks-dist-worker:latest \
		-f docker/Dockerfile.dist-worker docker

docker-image-tofino:
	docker build -t simbricks/simbricks:tofino \
		-f docker/Dockerfile.tofino .

include mk/subdir_post.mk
