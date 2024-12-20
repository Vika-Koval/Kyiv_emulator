// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <cstdint>
#include <cmath>
#include <cassert>
#include <bitset>
#include <fstream>
#include <regex>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/float128.hpp>
#include "kyiv.h"
#include "asm_disasm.h"
typedef uint64_t addr_t;
typedef uint64_t word_t;
typedef int64_t  signed_word_t;
typedef uint32_t opcode_t;
typedef boost::multiprecision::int128_t mul_word_t;


constexpr addr3_t word_to_addr3(word_t w){
    constexpr word_t Addr_1_mask_shift = (40-6-11)+1;
    constexpr word_t Addr_1_mask = 0b11'111'111'111ULL << (Addr_1_mask_shift);  // also was -1
    constexpr word_t Addr_2_mask_shift = (40-6-12-11)+1;
    constexpr word_t Addr_2_mask = 0b11'111'111'111ULL << (Addr_2_mask_shift);  // also was -1
    constexpr word_t Addr_3_mask_shift = 0; // Для одноманітності
    constexpr word_t Addr_3_mask = 0b11'111'111'111ULL;

    addr3_t res;
    res.source_1    = (w & Addr_1_mask) >> Addr_1_mask_shift;
    res.source_2    = (w & Addr_2_mask) >> Addr_2_mask_shift;
    res.destination = (w & Addr_3_mask);
    return res;
}

constexpr opcode_t word_to_opcode(word_t w) {
    constexpr word_t op_code_shift = (40-5)+1;
    constexpr word_t op_code_mask = 0b11'111ULL << (op_code_shift);
    opcode_t opcode = (w & op_code_mask) >> op_code_shift;
    return opcode;
}

static constexpr word_t mask_40_bits = (1ULL << 40) - 1; // 0b111...11 -- 40 1-bits, std::pow(2, 41) === 1 << 41
static constexpr word_t mask_41_bit = (1ULL << 40);      // 0b1000...00 -- 40 zeros after the 1


constexpr bool is_negative(word_t w){
    return w & mask_41_bit; // 0 - додатнє, не нуль -- від'ємне
}

constexpr word_t to_negative(word_t w){
    return w | mask_41_bit;
}

constexpr word_t to_positive(word_t w){
    return w & (~mask_41_bit);
}

constexpr uint16_t leftmost_one(word_t w){
    uint16_t ct = 0;
    while (w > 1) {
        ct++;
        w = w >> 1;
    }
    return ct;
}

//hz looks like kostyl but whatever
uint16_t leftmost_one(mul_word_t w){
    uint16_t ct = 0;
    while (w > 1) {
        ct++;
        w = w >> 1;
    }
    return ct;
}

signed_word_t get_absolute(word_t w){
    return static_cast<signed_word_t>(w & mask_40_bits);
}

signed_word_t word_to_number(word_t w){
    signed_word_t sign1 = (is_negative(w) ? -1 : 1);
    signed_word_t abs_val1 = get_absolute(w);
    return sign1 * abs_val1;
}

constexpr bool get_A1(word_t w){
    constexpr word_t A_1_mask = 1ULL << (40-5);
    return w & A_1_mask;
}

constexpr bool get_A2(word_t w){
    constexpr word_t A_2_mask = 1ULL << (40-5-12);
    return w & A_2_mask;
}

constexpr bool get_A3(word_t w){
    constexpr uint64_t A_3_mask = 1ULL << (40-5-24);
    return w & A_3_mask;
}

constexpr addr3_t shift_addr3_byA(addr3_t addr3, uint64_t offset, word_t w){
    if(get_A1(w))
        addr3.source_1 += offset;
    if(get_A2(w))
        addr3.source_2 += offset;
    if(get_A3(w))
        addr3.destination += offset;
    return  addr3;
}


//! Returns: True -- continue, false -- ситуація останову.
bool Kyiv_t::execute_opcode(){
    K_reg = kmem.read_memory(C_reg);
    std::cout << "Our command(binary): "<<std::bitset<41>(K_reg) << std::endl;
    std::cout << "Our command(Kyive): "<<K_reg << std::endl;
    opcode_t opcode = word_to_opcode(K_reg);
    std::cout<<"Opcode: "<<opcode<<std::endl;

    if (opcode == 0) {
        ++C_reg;
        std::cout << "No operation (opcode 0). Moving to the next command." << std::endl;
    }
//    std::cout << "opcode: " << opcode << std::endl;
    addr3_t addr3 = word_to_addr3(K_reg); // Парі команд потрібна
    std::cout << "A_reg_2: " << A_reg << std::endl;
    std::cout << "Cycle_reg: " << Loop_reg << std::endl;
    addr3_t addr3_shifted = shift_addr3_byA(addr3, A_reg, K_reg); // Решта використовують цю змінну

    std::cout << "Shifted addresses (source 1: " << addr3_shifted.source_1
          << ", source 2: " << addr3_shifted.source_2
          << ", destination: " << addr3_shifted.destination << ")" << std::endl;
    // std::cout << "source 1: " << addr3_shifted.source_1 << std::endl;
    // std::cout << "source 2: " << addr3_shifted.source_2 << std::endl;

    disassembly(K_reg, kmem, addr3_shifted);
    //! Ймовірно, потім це діло треба буде відрефакторити -- відчуваю, но де буде проблема - поки не знаю :+)
    switch(opcode){

        //TODO: Тестував лише opcode_add !!! -- решта вважайте невірними, поки не буде тестів. opcode_div
        case arythm_operations_t::opcode_div: [[fallthrough]];
        case arythm_operations_t::opcode_norm: [[fallthrough]];
        case arythm_operations_t::opcode_add: [[fallthrough]];
        case arythm_operations_t::opcode_sub: [[fallthrough]];
        case arythm_operations_t::opcode_addcmd: [[fallthrough]];
        case arythm_operations_t::opcode_subabs: [[fallthrough]];
        case arythm_operations_t::opcode_mul: [[fallthrough]];
        case arythm_operations_t::opcode_addcyc: [[fallthrough]];
        case arythm_operations_t::opcode_mul_round:
            opcode_arythm(addr3_shifted, opcode);
            break;
            //==========================================================================================================
        case flow_control_operations_t::opcode_jmp_less_or_equal: [[fallthrough]];
        case flow_control_operations_t::opcode_jmp_abs_less_or_equal: [[fallthrough]];
        case flow_control_operations_t::opcode_jmp_equal: [[fallthrough]];
        case flow_control_operations_t::opcode_fork_negative: [[fallthrough]];
        case flow_control_operations_t::opcode_call_negative: [[fallthrough]];
        case flow_control_operations_t::opcode_ret: [[fallthrough]];
        case flow_control_operations_t::opcode_group_op_begin: [[fallthrough]];
        case flow_control_operations_t::opcode_group_op_end: [[fallthrough]];
        case flow_control_operations_t::opcode_F: [[fallthrough]];
        case flow_control_operations_t::opcode_stop:
            opcode_flow_control(addr3_shifted, opcode, addr3);
            break;
//==========================================================================================================
        case logic_operations_t::opcode_log_shift:{
            //! TODO: Мені не повністю зрозуміло з обох книг, величина зсуву береться із RAM за адресою,
            //! чи закодована в команді? Швидше перше -- але перевірити!
            word_t shift = kmem.read_memory(addr3_shifted.source_1) ; // Глушко-Ющенко, стор 12, сверджує: "на число разрядов,
            // равное абсолютной величине константы сдвига, размещаемой в шести младших разрядах ячейки а1"
            // 2^6 -- 64, тому решта бітів справді просто дадуть нуль на виході, але все рівно маскую, щоб
            // не було невизначеної поведінки С. Та й зразу знак викидаємо
            std::cout << "shift: " << shift << std::endl;
            shift &= 0b111'111;
            std::cout << "shift after some and: " << shift << std::endl;
            if(is_negative(kmem.read_memory(addr3_shifted.source_1))) {
                kmem.write_memory(addr3_shifted.destination, kmem.read_memory(addr3_shifted.source_2) >> shift);
            } else {
                kmem.write_memory(addr3_shifted.destination , kmem.read_memory(addr3_shifted.source_2) << shift);
                kmem.write_memory(addr3_shifted.destination,  kmem.read_memory(addr3_shifted.destination) & (mask_40_bits | mask_41_bit)); // Зануляємо зайві біти
            }
            ++C_reg;
        }
            break;
        case logic_operations_t::opcode_log_or:{
            kmem.write_memory(addr3_shifted.destination, kmem.read_memory(addr3_shifted.source_1) | kmem.read_memory(addr3_shifted.source_2));
            ++C_reg;
            std::cout << "orr" << kmem.read_memory(addr3_shifted.destination) << std::endl;
        }
            break;
        case logic_operations_t::opcode_log_and:{

            kmem.write_memory(addr3_shifted.destination, kmem.read_memory(addr3_shifted.source_1) & kmem.read_memory(addr3_shifted.source_2));

            ++C_reg;
        }
            break;
        case logic_operations_t::opcode_log_xor:{
            kmem.write_memory(addr3_shifted.destination, kmem.read_memory(addr3_shifted.source_1) ^ kmem.read_memory(addr3_shifted.source_2));
            ++C_reg;
        }
            break;
//==========================================================================================================
        case IO_operations_t::opcode_read_perfo_data:{
                std::ifstream punch_cards;
                std::string line;
                std::string perfo;
                std::vector<std::string> argv;
                signed_word_t number;

                punch_cards.open("../../mem/punch_cards_in.txt");

                int counter = 0;
                int num_counter = 0;
                bool flag;

                while (punch_cards) {
                    std::getline(punch_cards, line);
                    if (counter == perfo_num) {
                        perfo = line.substr(addr3_shifted.destination, line.size());
                        boost::split(argv, perfo, boost::is_any_of(" "), boost::algorithm::token_compress_off);
                        for(auto num : argv){
                            if(num_counter == addr3_shifted.source_2 - addr3_shifted.source_1){
                                flag = true;
                                break;
                            }
                            number = std::stol(num);
                            if(number >= 0){
                                kmem.write_memory(addr3_shifted.source_1 + num_counter, number);
                            }else{
                                kmem.write_memory(addr3_shifted.source_1 + num_counter, to_negative(std::abs(number)));
                            }
                            num_counter++;
                        }
                    }else if(counter > num_counter){
                        perfo = line;
                        boost::split(argv, perfo, boost::is_any_of(" "), boost::algorithm::token_compress_off);
                        for(auto num : argv){
                            if(num_counter == addr3_shifted.source_2 - addr3_shifted.source_1){
                                flag = true;
                                break;
                            }
                            number = std::stoi(num);
                            if (number >= 0) {
                                kmem.write_memory(addr3_shifted.source_1 + num_counter, number);
                            } else {
                                kmem.write_memory(addr3_shifted.source_1 + num_counter, to_negative(std::abs(number)));
                            }
                            num_counter ++;
                        }
                    }
                    if(flag == true){
                        break;
                    }
                    counter++;
                }
                punch_cards.close();
        }
            break;

        case IO_operations_t::opcode_read_perfo_binary:{
            std::ifstream punch_cards;
            std::ifstream heads;
            std::string head;
            std::string line;

            punch_cards.open("../punched_tape.txt");
            heads.open("../heads.txt");

            std::getline(heads, head);
            std::getline(heads, head);
            // int num = std::stoi(head);
            size_t num = h;
            int counter = 0;
            int com_counter = 0;
            bool flag = false;

            while(punch_cards){
                std::getline(punch_cards, line);
                int pos = 0;
                if(counter == num){
                    if(com_counter < addr3_shifted.source_2 - addr3_shifted.source_1){
                        kmem.write_memory(addr3_shifted.source_1 + com_counter, std::stol(line, 0, 8));
                        com_counter++;
                    }else{
                        flag = true;
                        break;
                    }
                }else if(counter > num){
                    if(com_counter < addr3_shifted.source_2 - addr3_shifted.source_1){
                        kmem.write_memory(addr3_shifted.source_1 + com_counter, std::stol(line, 0, 8));
                        com_counter ++;
                    }else{
                        flag = true;
                        break;
                    }

                }
                if(flag){
                    break;
                }
            }
            h += com_counter;
        }
            break;

        case IO_operations_t::opcode_read_magnetic_drum:{
            std::ifstream magnetic_drum;
            std::string line;
            std::string data;
            std::vector<std::string> argv;
            signed_word_t number;

            magnetic_drum.open("../../mem/drum_in.txt");

            int counter = 0;
            int num_counter = 0;
            bool flag;
            while (magnetic_drum) {
                std::getline(magnetic_drum, line);
                if(counter == drum_num_read){
                    data = line.substr(drum_zone_read, line.size());
                    boost::split(argv, data, boost::is_any_of(" "), boost::algorithm::token_compress_off);
                    for(const auto& num : argv){
                        if(num_counter == addr3_shifted.source_2 - addr3_shifted.source_1){
                            flag = true;
                            break;
                        }
                        number = std::stoi(num);
                        if(number >= 0){
                            kmem.write_memory(addr3_shifted.source_1 + num_counter, number);
                        }else{
                            kmem.write_memory(addr3_shifted.source_1 + num_counter, to_negative(std::abs(number)));
                        }
                        num_counter ++;
                    }
                }else if(counter > drum_num_read){
                    data = line;
                    boost::split(argv, data, boost::is_any_of(" "), boost::algorithm::token_compress_off);
                    for(const auto& num : argv){
                        if(num_counter == addr3_shifted.source_2 - addr3_shifted.source_1){
                            flag = true;
                            break;
                        }
                        number = std::stoi(num);
                        if(number >= 0){
                            kmem.write_memory(addr3_shifted.source_1 + num_counter, number);
                        }else{
                            kmem.write_memory(addr3_shifted.source_1 + num_counter, to_negative(std::abs(number)));
                        }
                        num_counter ++;
                    }
                }
                if(flag){
                    break;
                }
                counter ++;
            }

            magnetic_drum.close();
        }
            break;

        case IO_operations_t::opcode_write_perfo_binary:{
            std::ofstream myfile;
            myfile.open("../punc_cards_out.txt");
            if (myfile.is_open())
            {
                for(uint64_t i = 0; i <= addr3_shifted.source_2; i++){
                    myfile << word_to_number(kmem.read_memory(addr3_shifted.source_1 + i));
                    myfile << ' ';
                }
                myfile.close();
            }else {
                std::cout << "Unable to open file";
            }
            C_reg = addr3_shifted.destination;
            K_reg = kmem.read_memory(C_reg);
        }


        case IO_operations_t::opcode_write_magnetic_drum:{
            std::ofstream myfile;
            myfile.open("../magnetic_drum.txt");
            if (myfile.is_open())
            {
                for(uint64_t i = 0; i <= addr3_shifted.source_2; i++){
                    myfile << word_to_number(kmem.read_memory(addr3_shifted.source_1 + i));
                    myfile << ' ';
                }
                myfile.close();
            }else {
                std::cout << "Unable to open file";
            }
            C_reg = addr3_shifted.destination;
            K_reg = kmem.read_memory(C_reg);
        }
            break;

        case IO_operations_t::opcode_init_magnetic_drum:{
            if (addr3_shifted.source_1 == 0) {
                drum_num_read = addr3_shifted.source_2;
                drum_zone_read = addr3_shifted.destination;
            } else if (addr3_shifted.source_1 == 1) {
                drum_num_write = addr3_shifted.source_2;
                drum_zone_write = addr3_shifted.destination;
            }
        }
            break;
//==========================================================================================================
        default:
            T_reg = true; // ! TODO: Не пам'ятаю, яка там точно реакція на невідому команду
    }
    return !T_reg;
}

void Kyiv_t::opcode_arythm(const addr3_t& addr3, opcode_t opcode){
    //! TODO: Додати перевірку на можливість запису. Що робила машина при спробі запису в ПЗП?
    //! TODO: Додати перевірку на вихід за границю пам'яті -- воно ніби зациклювалося при тому
    //! (зверталося до байта add mod 2^11, в сенсі), але точно не знаю.
    signed_word_t sign1 = (is_negative(kmem.read_memory(addr3.source_1)) ? -1 : 1);
    signed_word_t sign2 = (is_negative(kmem.read_memory(addr3.source_2)) ? -1 : 1);
    // std::cout << "sign 2" << sign2 << std::endl;
    // std::cout << "gfuigfalhf: " << addr3.source_2 << std::endl;
    word_t abs_val1 = static_cast<signed_word_t>(kmem.read_memory(addr3.source_1) & mask_40_bits);
    word_t abs_val2 = static_cast<signed_word_t>(kmem.read_memory(addr3.source_2) & mask_40_bits);
    signed_word_t res = sign1 * (signed_word_t) abs_val1;

    signed_word_t res_for_norm;
    mul_word_t res_mul;
    uint16_t power = 40 - leftmost_one(abs_val1) -1;

    // std::cout << sign1 * (signed_word_t) abs_val1 << "gifhdaflvdjasbh\t" << sign2 * (signed_word_t) abs_val2 << std::endl;
    std::cout << "Arithmetic operation: " << opcode
          << "\nSource 1: " << addr3.source_1 << " (value Kyive: " << sign1 * (signed_word_t)abs_val1 << ")"<< " (value decimal: " << sign1 * ((signed_word_t)abs_val1*pow(2, -40)) << ")"
          << "\nSource 2: " << addr3.source_2 << " (value Kyive: " << sign2 * (signed_word_t)abs_val2 << ")"<< " (value decimal: " << sign2 * ((signed_word_t)abs_val2*pow(2, -40)) << ")\n";
    switch(opcode){
        case arythm_operations_t::opcode_add:
            res += sign2 * (signed_word_t) abs_val2;
            std::cout << "Operation: Addition." << "\n";
            if (sign2==1){
                std::cout << "Kyive: " << sign1 * (signed_word_t) abs_val1 << " + " << sign2 * (signed_word_t) abs_val2 << " = " << res << std::endl;
                std::cout << "Decimal: " << sign1 * ((signed_word_t) abs_val1 * pow(2, -40)) << " + " << sign2 * ((signed_word_t) abs_val2 * pow(2, -40)) << " = " << res * pow(2, -40) << std::endl;
            } else {
                std::cout << "Kyive: " << sign1 * (signed_word_t) abs_val1 << " + " << "("<<sign2 * (signed_word_t) abs_val2 <<")"<< " = " << res << std::endl;
                std::cout << "Decimal: " << sign1 * ((signed_word_t) abs_val1 * pow(2, -40)) << " + "<< "(" << sign2 * ((signed_word_t) abs_val2 * pow(2, -40))<<")" << " = " << res * pow(2, -40) << std::endl;
            }
            break;
        case arythm_operations_t::opcode_sub:
            res -= sign2 * (signed_word_t) abs_val2;
            std::cout << "Operation: Subtraction." << "\n";
            if (sign2 == 1) {
                std::cout << "Kyive: " << sign1 * (signed_word_t) abs_val1 << " - " << sign2 * (signed_word_t) abs_val2 << " = " << res << std::endl;
                std::cout << "Decimal: " << sign1 * ((signed_word_t) abs_val1 * pow(2, -40)) << " - " << sign2 * ((signed_word_t) abs_val2 * pow(2, -40)) << " = " << res * pow(2, -40) << std::endl;
            } else {
                std::cout << "Kyive: " << sign1 * (signed_word_t) abs_val1 << " - " << "(" << sign2 * (signed_word_t) abs_val2 << ")" << " = " << res << std::endl;
                std::cout << "Decimal: " << sign1 * ((signed_word_t) abs_val1 * pow(2, -40)) << " - " << "(" << sign2 * ((signed_word_t) abs_val2 * pow(2, -40)) << ")" << " = " << res * pow(2, -40) << std::endl;
            }
            break;
        case arythm_operations_t::opcode_addcmd:
            res += (signed_word_t) abs_val2;
            std::cout << "Operation: Addition of commands." << "\n";
            std::cout<<"Add first number to second without sign: "<<"\n";
            std::cout << "Kyive: " << sign1 * (signed_word_t) abs_val1 << " + " <<  (signed_word_t) abs_val2 << " = " << res << std::endl;
            std::cout << "Decimal: " << sign1 * ((signed_word_t) abs_val1 * pow(2, -40)) << " + " <<  ((signed_word_t) abs_val2 * pow(2, -40)) << " = " << res * pow(2, -40) << std::endl;
            break;
        case arythm_operations_t::opcode_subabs:
            res = (signed_word_t) abs_val1 - (signed_word_t) abs_val2;
            std::cout << "Operation: Modulus substraction" << "\n";
            std::cout << "Kyive: " << "|"<<sign1 * (signed_word_t) abs_val1 << "|"<< " - " << "|" << sign2 * (signed_word_t) abs_val2 << "|" << " = " << res << std::endl;
            std::cout << "Decimal: " << "|"<< sign1 * ((signed_word_t) abs_val1 * pow(2, -40)) << "|"<< " - " << "|"<< sign2 * ((signed_word_t) abs_val2 * pow(2, -40))<< "|"<< " = " << res * pow(2, -40) << std::endl;
            break;
        case arythm_operations_t::opcode_addcyc:
            res += sign2 * (signed_word_t) abs_val2; // Те ж, що і для opcode_add, але подальша обробка інша
            std::cout << "Operation: Cyclical addition." << "\n";
            if (sign2==1){
                std::cout << "Kyive: " << sign1 * (signed_word_t) abs_val1 << " + " << sign2 * (signed_word_t) abs_val2 << " = " << res << std::endl;
                std::cout << "Decimal: " << sign1 * ((signed_word_t) abs_val1 * pow(2, -40)) << " + " << sign2 * ((signed_word_t) abs_val2 * pow(2, -40)) << " = " << res * pow(2, -40) << std::endl;
            } else {
                std::cout << "Kyive: " << sign1 * (signed_word_t) abs_val1 << " + " << "("<<sign2 * (signed_word_t) abs_val2 <<")"<< " = " << res << std::endl;
                std::cout << "Decimal: " << sign1 * ((signed_word_t) abs_val1 * pow(2, -40)) << " + "<< "(" << sign2 * ((signed_word_t) abs_val2 * pow(2, -40))<<")" << " = " << res * pow(2, -40) << std::endl;
            }
            std::cout<<"In the end we check whether our result was negative, and add - in case if yes"<<std::endl;
            break;
        case arythm_operations_t::opcode_mul: [[fallthrough]];
        case arythm_operations_t::opcode_mul_round:
            res_mul = sign1 * (mul_word_t) abs_val1 * sign2 * (mul_word_t) abs_val2;
            std::cout << "H : " << res_mul << std::endl;
            break;
        case arythm_operations_t::opcode_norm: {
            res_for_norm = sign1 * (abs_val1 << power);
        }
            break;
        case arythm_operations_t::opcode_div: {
            if ((abs_val2 == 0) || (abs_val2 < abs_val1)) {
                T_reg = true;
                ++C_reg;
                return;
            }
            res_mul = ((mul_word_t) abs_val1 << 40) / (mul_word_t) abs_val2;
            // std::cout << "Div " << res_mul << std::endl;
        }
            break;

        default:
            assert(false && "Should never been here!");
    }

    if(opcode == arythm_operations_t::opcode_add ||
       opcode == arythm_operations_t::opcode_sub ||
       opcode == arythm_operations_t::opcode_subabs     //! TODO: Я не плутаю, результат може мати знак?
            ) {
        //! TODO: До речі, а якщо переповнення, воно кінцевий регістр змінювало до останову, чи ні?
        // Тут я зробив, ніби ні -- але ХЗ, могло. Щоб точно знати -- треба моделювати на рівні схем ;=) --
        // як ви і поривалися. Але це не має бути важливим.
        bool is_negative = (res < 0);
        if (is_negative)
            res = -res;
        assert(res >= 0);
        if (res & mask_41_bit) { // if sum & CPU1.mask_41_bit == 1 -- overflow to sign bit
            T_reg = true;
            ++C_reg;
            return;
        }
        kmem.write_memory(addr3.destination, static_cast<uint64_t>(res) & mask_40_bits);
        // std::cout << -1 * res << std::endl;
        if (is_negative)
            kmem.write_memory(addr3.destination, kmem.read_memory(addr3.destination) | mask_41_bit);
        //! "Нуль, получаемый как разность двух равных чисел, имеет отрицательный знак" -- стор. 13 Глушко-Ющенко, опис УПЧ
        if(opcode == arythm_operations_t::opcode_sub && res == 0
           && abs_val2 == 0 //! TODO: Моє припущення -- перевірити!
                ){
            kmem.write_memory(addr3.destination, res | mask_41_bit);
            std::cout << "NEGATIVE 0" << (res | mask_41_bit) << std::endl;
        }
    } else if(opcode == arythm_operations_t::opcode_addcmd){
        kmem.write_memory(addr3.destination, static_cast<uint64_t>(res) & mask_40_bits);
        if (res<0) {
            std::cout<<"Since our result is negative,we will add 1 to our result:"<<"1 +"<<"("<<res<<")"<<" = "<<(kmem.read_memory(addr3.destination) & mask_40_bits)<<std::endl;
        }
        std::cout << "Result written to destination (without sign): "
          << (kmem.read_memory(addr3.destination) & mask_40_bits)
          << " (Decimal: " << word_to_number(kmem.read_memory(addr3.destination)) * pow(2, -40) << ")" << std::endl;
        kmem.write_memory(addr3.destination, kmem.read_memory(addr3.destination) | (kmem.read_memory(addr3.source_2) & mask_41_bit)); // Копіюємо біт знаку з source_2 // edited тут наче так має бути
        std::cout << "Final result written to destination (with sign): "
          << kmem.read_memory(addr3.destination)
          << " (Decimal: " << word_to_number(kmem.read_memory(addr3.destination)) * pow(2, -40) << ")" << std::endl;
    } else if(opcode == arythm_operations_t::opcode_addcyc){
        //! TODO: Вияснити, а як ця команда функціонує.
        // "Отличается от обычного сложения лишь тем, что  в нем отсутствует блокировка при выходе
        // из разполагаемого числа разрядов. Перенос из знакового разряда поступает в младший разряд
        // сумматора".
        // Питання (нумеруючи біти з 1 до 41):
        // 1. Перенос із 40 в 41 біт тут можливий? З фрази виглядає, що так.
        // 2. Якщо додавання переносу до молодшого біту виникло переповнення, що далі?
        //    Так виглядає, що воно не може виникнути, але чи я не помилився? -- не може, десь через переніс буде 0
        bool is_negative = (res < 0);
        if (is_negative)
            res = -res;
        assert(res >= 0);

        // std::cout << std::bitset<41> (res) << std::endl;
        if(res & mask_41_bit){
            res += 1; // Маємо перенос із знакового біту
        }
        kmem.write_memory(addr3.destination, static_cast<uint64_t>(res) & mask_40_bits);

        std::cout<<"We will work with positive numbers"<<" (value Kyive: " << res << ")"<< " (value decimal: " << res*pow(2, -40) << ")"<<std::endl;
        if (res*pow(2, -40)>1 ) {
            std::cout<<"Our result out of range [-1,1]"<<std::endl;
            std::cout<<"We need to sustruct 1 from our result Kyive:"<<res<<" - 1"<<" = "<< (kmem.read_memory(addr3.destination) & mask_40_bits)<<std::endl;
            std::cout<<"We need to sustruct 1 from our result decimal:"<<res*pow(2, -40)<<" - 1"<<" = "<< (kmem.read_memory(addr3.destination) & mask_40_bits)*pow(2, -40)<<std::endl;
        }
        else {
            std::cout<<"Our value belong to reange [-1,1], so it will stay unchanged"<<" (value Kyive: " << res << ")"<< " (value decimal: " << res*pow(2, -40) << ")"<<std::endl;
        }
        if (is_negative)
            kmem.write_memory(addr3.destination, kmem.read_memory(addr3.destination) | mask_41_bit);

    } else if(opcode == arythm_operations_t::opcode_mul ||
              opcode == arythm_operations_t::opcode_mul_round
            ) {
        bool is_negative = (res_mul < 0);
        //std::cout << res_mul << std::endl;
        if (is_negative)
            res_mul = -res_mul;
        assert(res_mul >= 0);

        uint16_t leftmost = leftmost_one(res_mul);

        if (opcode == arythm_operations_t::opcode_mul_round) {
            res_mul += 1ULL << 39;
        }
        res_mul = res_mul >> 40;

        kmem.write_memory(addr3.destination, static_cast<uint64_t>(res_mul) & mask_40_bits);
        std::cout << "DEBUUUUUUUUG2 " << static_cast<uint64_t>(res_mul) << std::endl;
        // std::cout << is_negative << std::endl;
        if (is_negative)
            kmem.write_memory(addr3.destination, kmem.read_memory(addr3.destination) | mask_41_bit);
        std::cout << "DEBUUUUUUUUG " << word_to_number(kmem.read_memory(addr3.destination)) << std::endl;
//        std::cout << std::bitset<41>(kmem[addr3.destination]) << std::endl;
//        std::cout << "Mult res: " << word_to_number(kmem[addr3.destination]) << std::endl;

    } else if (opcode == arythm_operations_t::opcode_norm) {
        bool is_negative = (res_for_norm < 0);

        if (is_negative)
            res_for_norm = -res_for_norm;
        assert(res_for_norm >= 0);
//
//        std::cout << "norm_val: " << (res_for_norm) << std::endl;
//        std::cout << "norm_power: " << (power) << std::endl;
//        std::cout << "norm_val_64: " << std::bitset<64>(res_for_norm) << std::endl;
//        std::cout << "norm_val_41: " << std::bitset<41>(res_for_norm) << std::endl;

        kmem.write_memory(addr3.source_2, power);
        kmem.write_memory(addr3.destination, static_cast<uint64_t>(res_for_norm) & mask_40_bits);
        if (is_negative)
            kmem.write_memory(addr3.destination, kmem.read_memory(addr3.destination) | mask_41_bit);
//
//        std::cout << "norm_val_mem: " << kmem[addr3.destination] << std::endl;
//        std::cout << "norm_val_pow: " << kmem[addr3.source_2] << std::endl;
    } else if (opcode == arythm_operations_t::opcode_div) {
        kmem.write_memory(addr3.destination, static_cast<uint64_t>(res_mul) & mask_40_bits);
        if ((sign1 * sign2) == -1)
            kmem.write_memory(addr3.destination, kmem.read_memory(addr3.destination) | mask_41_bit);
//        std::cout << "Div res: " << word_to_number(kmem[addr3.destination]) << std::endl;
    }
    ++C_reg;
}


void Kyiv_t::opcode_flow_control(const addr3_t& addr3_shifted, opcode_t opcode, const addr3_t &addr3){
    signed_word_t sign1 = (is_negative(kmem.read_memory(addr3_shifted.source_1)) ? -1 : 1);
    signed_word_t sign2 = (is_negative(kmem.read_memory(addr3_shifted.source_2)) ? -1 : 1);
    signed_word_t abs_val1 = static_cast<signed_word_t>(kmem.read_memory(addr3_shifted.source_1) & mask_40_bits);
    signed_word_t abs_val2 = static_cast<signed_word_t>(kmem.read_memory(addr3_shifted.source_2) & mask_40_bits);;

    switch (opcode) {
        case flow_control_operations_t::opcode_jmp_less_or_equal: {
            if((sign1 * abs_val1) <= (sign2 * abs_val2)){
                C_reg = addr3_shifted.destination;
            } else {
                ++C_reg;
            }
        }
            break;
        case flow_control_operations_t::opcode_jmp_abs_less_or_equal: {
//            std::cout << "Num1 " << get_absolute(kmem[addr3_shifted.source_1]) << std::endl;
//            std::cout << "Num2 " << get_absolute(kmem[addr3_shifted.source_2]) << std::endl;
            if (abs_val1 <= abs_val2) {
                C_reg = addr3_shifted.destination;
            } else {
                ++C_reg;
            }
        }
            break;
        case flow_control_operations_t::opcode_jmp_equal: {
            if( (sign1 * abs_val1) == (sign2 * abs_val2)){
                C_reg = addr3_shifted.destination;
            } else {
                ++C_reg;
            }
        }
            break;
        case flow_control_operations_t::opcode_fork_negative: {
            if( is_negative(kmem.read_memory(addr3_shifted.source_1)) ){
                C_reg = addr3_shifted.destination;
            }else{
                C_reg = addr3_shifted.source_2;
            }
        }
            break;
        case flow_control_operations_t::opcode_call_negative:{
            if( is_negative(kmem.read_memory(addr3_shifted.source_1)) ){
                P_reg = addr3_shifted.source_2; //! TODO: Згідно тексту стор 342 (пункт 18) Гнеденко-Королюк-Ющенко-1961
                //! Глушков-Ющенко, стор 13, УПП не до кінця однозначна -- a1 без штриха, це зрозуміло,
                //! але з врахуванням A і біта модифікатора, чи без?
                //! Виглядає, що в таблиці на стор 180 -- помилка ('A2 => P -- зайвий штрих точно помилка,
                //! чи помилка, що, немає зсуву на А?).
                //! Однак, в  Гнеденко-Королюк-Ющенко-1961 опкод (32) суперечить опкоду в Глушко-Ющенко.
                //! Перевірити!
                C_reg = addr3_shifted.destination;
            }else{
                ++C_reg; //! Тут P_reg не мала б змінювати
            }
        }
            break;
        case flow_control_operations_t::opcode_ret:{
            C_reg = P_reg;
        }
            break;
        case flow_control_operations_t::opcode_group_op_begin:{
            // У книжці Глушков-Ющенко на ст. 14, ймовірно, помилка, бо навіть словами пояснено,
            // що береться значення а1 і а2, але разом із тим наголошено, що береться не 'а1 чи 'а2,
            // а саме а1 і а2. В кінці цієї книжки та у Гнеденко-Королюк-Ющенко пише,
            // ніби беруться значення, тому тут реалізовано саме так.
            Loop_reg =  addr3_shifted.source_1; //word_to_number(kmem.read_memory(addr3_shifted.source_1));
            A_reg = addr3_shifted.source_2; // word_to_number(kmem.read_memory(addr3_shifted.source_2));
            if (A_reg == Loop_reg) {
                C_reg = addr3_shifted.destination;
            } else {
                ++C_reg;
            }

            std::cout << "A_reg: " << A_reg << std::endl;
        }
            break;
        case flow_control_operations_t::opcode_group_op_end:{
            // Такий самий прикол, як з НГО
            // not a value
            A_reg += addr3.source_1;// word_to_number(kmem.read_memory(addr3_shifted.source_1));
            if (A_reg == Loop_reg) {
                C_reg = word_to_number(addr3_shifted.destination);
            } else {
                C_reg = (addr3_shifted.source_2);
            }
        }
            break;
        case flow_control_operations_t::opcode_F:{
            // Якщо я правильно розібралася з 2 попередними командами, то тут все зрозуміло і немає суперечностей
            A_reg = word_to_addr3(kmem.read_memory(addr3_shifted.source_1)).source_2;
            word_t res = kmem.read_memory(A_reg);
            kmem.write_memory(addr3_shifted.destination, res);
            ++C_reg;
        }
            break;
        case flow_control_operations_t::opcode_stop:{ //! TODO: Вона враховує стан кнопки на пульті?
            // From Glushkov-Iushchenko p. 55
            // If B_tumb == 0 -> neutral mode -> full stop
            // If B_tumb > 0 -> just skip one command without full stop
            // From Glushkov-Iushchenko pp. 163-164
            // If B_tumb == 1 -> stop by 3d address
            // If B_tumb == 2 -> stop by command number
            // I'm not sure what to do with 1st and 2nd B_tumb (maybe that should be handled in main???)
            if (!B_tumb) {
                T_reg = true;
                ++C_reg;
            }
            else {
                C_reg += 2;
            }
        }
            break;
    }
}
