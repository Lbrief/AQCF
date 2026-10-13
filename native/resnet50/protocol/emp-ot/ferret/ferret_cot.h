#ifndef EMP_FERRET_COT_H_
#define EMP_FERRET_COT_H_
#include "resnet50/vendor/emp/emp-ot/ferret/mpcot_reg.h"
#include "resnet50/vendor/emp/emp-ot/ferret/base_cot.h"
#include "resnet50/protocol/emp-ot/ferret/lpn_f2.h"
#include "resnet50/vendor/emp/emp-ot/ferret/constants.h"

#include <memory>
#include "resnet50/protocol/pool_correlation_policy.hpp"
#include <array>
namespace emp {

/*
 * Ferret COT binary version
 * [REF] Implementation of "Ferret: Fast Extension for coRRElated oT with small communication"
 * https://eprint.iacr.org/2020/924.pdf
 *
 */
template<typename T>
class FerretCOT: public COT<T> { 
public:
	using COT<T>::io;
	using COT<T>::Delta;

	int64_t n, t, k, log_bin_sz;
	int64_t n_pre, t_pre, k_pre, log_bin_sz_pre;
	int64_t ot_used, ot_limit;

	FerretCOT(int party, int threads, T **ios, bool malicious = false, bool run_setup = true, std::string pre_file="");

	~FerretCOT();

	void setup(block Deltain, std::string pre_file = "");

	void setup(std::string pre_file = "");

	void send_cot(block * data, int64_t length) override;

	void recv_cot(block* data, const bool * b, int64_t length) override;


 int64_t queued315() const {return fifo315_size+int64_t(reserve443.size());}
 void prefill315();
 void rcot(block *data, int64_t num);

	int64_t rcot_inplace(block *ot_buffer, int64_t length);

	int64_t byte_memory_need_inplace(int64_t ot_need);

	void assemble_state(void * data, int64_t size);

	int disassemble_state(const void * data, int64_t size);

	int64_t state_size();
private:
 fifo443::Queue<block> reserve443;
 struct Fifo315 {std::unique_ptr<block[]> data;int64_t count=0,used=0;};
 std::array<Fifo315,2> fifo315;
 size_t fifo315_head=0,fifo315_tail=0;int64_t fifo315_size=0;
 void wipe_fifo315(){for(auto& c:fifo315){if(c.data){OPENSSL_cleanse(c.data.get(),size_t(c.count)*sizeof(block));c.data.reset();}c.count=c.used=0;}fifo315_head=fifo315_tail=0;fifo315_size=0;}
 block ch[2];

	T **ios;
	int party, threads;
	int64_t M;
	bool is_malicious;
	bool extend_initialized;

	block one;

	block * ot_pre_data = nullptr;
	block * ot_data = nullptr;

	std::string pre_ot_filename;

	BaseCot<T> *base_cot = nullptr;
	OTPre<T> *pre_ot = nullptr;
	ThreadPool *pool = nullptr;
	MpcotReg<T> *mpcot = nullptr;
	LpnF2<T, 10> *lpn_f2 = nullptr;

	
	void online_sender(block *data, int64_t length);

	void online_recver(block *data, const bool *b, int64_t length);

	void set_param();

	void set_preprocessing_param();

	void extend_initialization();

	void extend(block* ot_output, MpcotReg<T> *mpfss, OTPre<T> *preot, 
			LpnF2<T, 10> *lpn, block *ot_input);

	void extend_f2k(block *ot_buffer);

	void extend_f2k();

	int64_t silent_ot_left();

	void write_pre_data128_to_file(void* loc, __uint128_t delta, std::string filename);

	__uint128_t read_pre_data128_from_file(void* pre_loc, std::string filename);
};

#include "resnet50/protocol/emp-ot/ferret/ferret_cot.hpp"
}
#endif// _VOLE_H_
