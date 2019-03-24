#include <stdint.h>
#include <vector>

// 下記URLのソースを加工したもの
// https://www.ipa.go.jp/security/rfc/RFC3174JA.html

constexpr size_t HASHSIZE_IN_BYTES = 20;

/**
  * @brief SHA1 ハッシュ値の計算
  */
class SHA1
{
public:
	using hash_type = std::vector<uint8_t>;

public:
	// コンストラクタ
	SHA1();
	// デストラクタ
	~SHA1() = default;

	// データを追加する
	bool Add(const uint8_t* data, uint32_t dataLen);
	// データを確定する
	bool Final(hash_type& output);

private:
	bool Flush();
	void ProcessMessageBlock();

private:
	//! バッファ
	std::vector<uint8_t> m_buffer;
	//! 中間ハッシュ
	uint32_t m_intermediateHash[HASHSIZE_IN_BYTES / 4];
	//! 下位メッセージ長(ビット単位)
	uint32_t m_lowLen;
	//! 上位メッセージ長(ビット単位)
	uint32_t m_highLen;

	//! メッセージブロック(512bit)
	uint8_t m_msgBlock[64];
	//! 現在位置
	int32_t m_msgBlockIndex;

	//! ハッシュは計算済みか？
	bool m_computed;
	//! 破損フラグ
	bool m_currupted;
};


/**
  * @brief デフォルトコンストラクタ
  */
SHA1::SHA1() :
	m_lowLen(0), m_highLen(0), m_msgBlockIndex(0),
	m_computed(false), m_currupted(false)
{
	// H0～H1の初期化
	m_intermediateHash[0] = 0x67452301;
	m_intermediateHash[1] = 0xEFCDAB89;
	m_intermediateHash[2] = 0x98BADCFE;
	m_intermediateHash[3] = 0x10325476;
	m_intermediateHash[4] = 0xC3D2E1F0;
}

/**
  * @brief データを追加する
  * 
  * @return true: 成功  false: エラー
  * @param [in] data    ハッシュ化対象データ
  * @param [in] dataLen データ長(バイト単位)
  */
bool SHA1::Add(
	const uint8_t* data,
	uint32_t dataLen
)
{
	if (data == nullptr || dataLen == 0) {
		return true;
	}

	m_buffer.insert(m_buffer.end(), data, data + dataLen);
	if (m_buffer.size() >= 0xffff) {
		return Flush();
	}
	return true;
}

/**
  * @brief ハッシュ値を確定する
  * 
  * @return true: 成功  false: エラー
  * @param [out] output ハッシュ値
  */
bool SHA1::Final(
	hash_type& output
)
{
	if (m_currupted) {
		return false;
	}

	if (m_buffer.size() > 0) {
		Flush();
	}

	output.resize(HASHSIZE_IN_BYTES);

	if (m_computed == false) {

		if (m_msgBlockIndex > 55) {
			m_msgBlock[m_msgBlockIndex++] = 0x80;

			while (m_msgBlockIndex < 64) {
				m_msgBlock[m_msgBlockIndex++] = 0;
			}

			ProcessMessageBlock();
			while (m_msgBlockIndex < 56) {
				m_msgBlock[m_msgBlockIndex++] = 0;
			}
		}
		else {
			m_msgBlock[m_msgBlockIndex++] = 0x80;

			while (m_msgBlockIndex < 56) {
				m_msgBlock[m_msgBlockIndex++] = 0;
			}
		}

		// Store the message length as the last 8 octets
		m_msgBlock[56] = m_highLen >> 24;
		m_msgBlock[57] = m_highLen >> 16;
		m_msgBlock[58] = m_highLen >> 8;
		m_msgBlock[59] = m_highLen >> 0;
		m_msgBlock[60] = m_lowLen >> 24;
		m_msgBlock[61] = m_lowLen >> 16;
		m_msgBlock[62] = m_lowLen >> 8;
		m_msgBlock[63] = m_lowLen >> 0;

		ProcessMessageBlock();

		for (int i = 0; i < 64; ++i) {
			m_msgBlock[i] = 0;
		}

		m_lowLen = 0;
		m_highLen = 0;
		m_computed = true;
	}

	for (int i = 0; i < HASHSIZE_IN_BYTES; ++i) {
		output[i] = m_intermediateHash[i >> 2] >> 8 * (3 - (i & 0x03));
	}

	return true;
}



//! メッセージブロックの処理
void SHA1::ProcessMessageBlock()
{
	auto CircularShift = [](int bits, uint32_t word) {
		return ((word) << (bits)) | ((word) >> (32 - (bits)));
	};

	// SHA-1では、定数ワードのシーケンス K(0), K(1), ... , K(79) を使用する。
	// それぞれは、16 進表記で以下のように定義される。
	// K(t) = 5A827999 ( 0 <= t <= 19)
	// K(t) = 6ED9EBA1 (20 <= t <= 39)
	// K(t) = 8F1BBCDC (40 <= t <= 59)
	// K(t) = CA62C1D6 (60 <= t <= 79)
	constexpr uint32_t K0 = 0x5A827999;
	constexpr uint32_t K1 = 0x6ED9EBA1;
	constexpr uint32_t K2 = 0x8F1BBCDC;
	constexpr uint32_t K3 = 0xCA62C1D6;

	// a. M(i)を、16個のワード W(0), W(1), ... , W(15) に分割する。
	//    ここで W(0) は 最上位ワードである。
	uint32_t W[80];
	for (int t = 0; t < 16; t++) {
		W[t] = m_msgBlock[t * 4 + 0] << 24;
		W[t] |= m_msgBlock[t * 4 + 1] << 16;
		W[t] |= m_msgBlock[t * 4 + 2] << 8;
		W[t] |= m_msgBlock[t * 4 + 3] << 0;
	}

	// b. 16から79までのtに対して、以下の計算を行う。
	// W(t) = S^1(W(t-3) XOR W(t-8) XOR W(t-14) XOR W(t-16))
	for (int t = 16; t < 80; t++) {
		W[t] = CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
	}

	// Word buffers
	uint32_t A = m_intermediateHash[0];
	uint32_t B = m_intermediateHash[1];
	uint32_t C = m_intermediateHash[2];
	uint32_t D = m_intermediateHash[3];
	uint32_t E = m_intermediateHash[4];

	// d. 0から79までのtに対して、以下の計算を行う。
	for (int t = 0; t < 20; t++) {
		uint32_t temp = CircularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K0;
		E = D;
		D = C;
		C = CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (int t = 20; t < 40; t++) {
		uint32_t temp = CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K1;
		E = D;
		D = C;
		C = CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (int t = 40; t < 60; t++) {
		uint32_t temp = CircularShift(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K2;
		E = D;
		D = C;
		C = CircularShift(30, B);
		B = A;
		A = temp;
	}

	for (int t = 60; t < 80; t++) {
		uint32_t temp = CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K3;
		E = D;
		D = C;
		C = CircularShift(30, B);
		B = A;
		A = temp;
	}

	// e. HO, H1, H2, H3, H4 をそれぞれ、
	//    H0 = H0 + A, H1 = H1 + B, H2 = H2 + C, H3 = H3 + D, H4 = H4 + E とする。
	m_intermediateHash[0] += A;
	m_intermediateHash[1] += B;
	m_intermediateHash[2] += C;
	m_intermediateHash[3] += D;
	m_intermediateHash[4] += E;
	m_msgBlockIndex = 0;

	return;
}

//! バッファにたまったデータを投入する
bool SHA1::Flush()
{
	if (m_computed || m_currupted) {
		m_currupted = true;
		return false;
	}

	uint8_t* data = &m_buffer.front();
	uint32_t dataLen = (uint32_t)m_buffer.size();

	while (dataLen-- && m_currupted == false) {

		m_msgBlock[m_msgBlockIndex++] = (*data & 0xFF);
		m_lowLen += 8;
		if (m_lowLen == 0) {
			m_highLen++;
			if (m_highLen == 0) {
				// オーバーフロー
				m_currupted = 1;
			}
		}

		if (m_msgBlockIndex == 64) {
			ProcessMessageBlock();
		}
		data++;
	}

	m_buffer.clear();
	return true;
}


