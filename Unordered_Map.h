/**
 * @file Unordered_Map.h
 * @author Xukum
 * @brief My version of unordered map
 * @version 1.0
 * @date 2021-11-25
 * 
 * @copyright Copyright (c) 2021
 * 
 */


#pragma once
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <iostream>

/**
 * @brief Класс Unordered_Map
 * 
 * @tparam Key 
 * @tparam Value 
 */
template <typename Key, typename Value>
class Unordered_Map {

private:
	/**
	 * \private
	 * @brief Сокращение, принятое в пределах файла
	 */
	using U_Map = Unordered_Map<Key, Value>;
	template <typename Item> class OwnIterator;

	/**
	 * \private
	 * @brief Струкура узла, в котором хранится информация внутри таблицы
	 */
	struct Node {
		/**
		 * \private
		 * @brief Указатель на следующий за данным узел
		 * 
		 */
		Node* next_node;
		/**
		 * @brief Значение хеша от ключа, которому соответствует данный узел
		 * \private
		 */
		size_t cached;
		/**
		 * @brief Полезная информация (ключ-значение)
		 * \private
		 */
		std::pair<const Key, Value> value;

		Node(const Key& key, const Value& value, size_t cache) : value(key, value), cached(cache) {}
		Node(const std::pair<Key, Value>& value, size_t cache) : value(value), cached(cache) {}
		Node(const Node& other) : value(other.value), cached(other.cached) {}

		Node& operator=(const Node& other) {
			value = other.value; 
			cached = other.cached;
			next_node = other.next_node;
		}
	};
	/**
	 * @brief Специальный элемент для связи всех узлов в общий лист
	 * \private
	 */
	Node* Elist;
	/** \private 
	 * @brief Количество элементов в таблице 
	 */
	size_t _size;
	/**
	 * \private
	 * @brief Количество "корзин" в хеш-таблице
	 * 
	 * Количество "корзин" в хеш-таблице. Увеличивается автоматически по мере заполнения таблицы, или может 
	 * быть изменено с помощью функции reserve 
	 * \sa void reserve(size_t new_buckets_count)
	 */
	size_t buckets_count;
	
	/**
	 * \private
	 * @brief Массив "корзин"
	 * 
	 */
	Node** buckets;

	/**
	 * \private
	 * @brief Хеш-функция
	 * 
	 */
	std::hash<Key> hasher;
	/**
	 * \private
	 * @brief Коэффициент максимальной загруженности
	 * 
	 * При превышении этого коэффициента произойдёт 
	 * выделение памяти под большее количество корзин и таблица будет реорганизована (для уменьшения числа коллизий)
	 * 
	 * \sa double max_load_factor()
	 * \sa void max_load_factor(double f)
	 */
	double _max_load_factor = 1; 


public:
	/**
	 * @brief Construct a new Unordered_Map object
	 * @param num_of_buckets - количество "корзин" (по умолчанию = 10)
	 * 
	 */
	Unordered_Map(size_t num_of_buckets = 10) : _size(0), buckets_count(num_of_buckets) {
		buckets = new Node * [num_of_buckets];
		for (size_t i = 0; i < num_of_buckets; ++i) {
			buckets[i] = nullptr;
		}
		__create_Elist();
	}

	/**
	 * Construct a new Unordered_Map object based on another object (move)
	 * 
	 * @param other Объект, который будет перемещён в текущий экземпляр Unordered_Map
	 */
	Unordered_Map(U_Map&& other) noexcept :
		buckets(other.buckets), Elist(other.Elist), 
		buckets_count(other.buckets_count), _size(other._size), 
		_max_load_factor(other._max_load_factor), hasher(other.hasher)
	{
		other.buckets = nullptr;
		other.Elist = nullptr;
	}
	
	/**
	 * @brief Operator= based on moving
	 * 
	 * @return U_Map& ссылка на текущий экземпляр
	 */
	U_Map& operator=(U_Map&& other) noexcept {
		if (this == &other) return *this;

		clear();
		delete[] buckets;
		delete[] reinterpret_cast<int8_t*>(Elist);

		buckets = other.buckets;
		Elist = other.Elist;
		buckets_count = other.buckets_count;
		_size = other._size;
		_max_load_factor = other._max_load_factor;
		hasher = other.hasher;
		other.buckets = nullptr;
		other.Elist = nullptr;

		return *this;
	}

	/**
	 * Construct a new Unordered_Map object based on another object (copy)
	 * 
	 * @param other Объект, который будет перемещён в текущий экземпляр Unordered_Map
	 */
	Unordered_Map(const U_Map& other):
	buckets_count(other.buckets_count), _size(other._size), 
	_max_load_factor(other._max_load_factor), hasher(other.hasher) 
	{
		__create_Elist();
		buckets = new Node*[buckets_count];
		for (size_t i = 0; i < buckets_count; ++i) {
			buckets[i] = nullptr;
		}

		for (Node* it = other.Elist->next_node; it != other.Elist; it = it->next_node) {
			Node* new_node = new Node(*it);
			if (buckets[new_node->cached] == nullptr)
				__insert_in_empty_bucket(new_node->cached, new_node);
			else
				__insert_in_nonempty_bucket(new_node->cached, new_node);
		}
	}	
	
	/**
	 * @brief Operator= based on copying
	 * 
	 * @return U_Map& ссылка на текущий экземпляр
	 */
	U_Map& operator= (const U_Map& other){
		if (this == &other) return *this;

		U_Map tmp = other;

		clear();
		delete[] buckets;
		delete[] reinterpret_cast<int8_t*>(Elist);

		buckets = tmp.buckets;
		Elist = tmp.Elist;
		buckets_count = tmp.buckets_count;
		_size = tmp._size;
		_max_load_factor = tmp._max_load_factor;
		hasher = tmp.hasher;
		tmp.buckets = nullptr;
		tmp.Elist = nullptr;

		return *this;
	}

	/**
	 * @brief Destroy the Unordered_Map object
	 * 
	 */
	~Unordered_Map() {
		clear();
		delete[] reinterpret_cast<int8_t*>(Elist);
		delete[] buckets;
	}


public:
	using iterator = OwnIterator<std::pair<const Key, Value>>; ///< iterator definition
	using const_iterator = OwnIterator<const std::pair<const Key, Value>>; ///< const_iterator definition

	/** Returns iterator on begin of table*/
	iterator begin() {
		return iterator(Elist->next_node);
	}
	/** Returns const_iterator on begin for a const unordered_map*/
	const_iterator begin() const {
		return const_iterator(Elist->next_node);
	}

	/** Return iterator on end of table*/
	iterator end() {
		return iterator(Elist);
	}

	/** Returns const_iterator on end for a const unordered_map*/
	const_iterator end() const {
		return const_iterator(Elist);
	}

	/** @brief Returns const_iterator on begin of table*/
	const_iterator cbegin() const { 
		return const_iterator(Elist->next_node);
	}

	/** @brief Returns const_iterator on end of table*/
	const_iterator cend() const {
		return const_iterator(Elist);
	}



private:
	void __create_Elist() {
		Elist = reinterpret_cast<Node*>(new int8_t[sizeof(Node)]);
		Elist->next_node = Elist;
	}

	size_t __hash(const Key& key) const {
		if (buckets_count == 0) {
			throw std::invalid_argument("Impossible to find hash for zero-sized map");
		}
		return hasher(key) % buckets_count;
	}

	bool __find_in_bucket(size_t buck, const Key& key) const noexcept {
		if (buckets[buck] == nullptr) return false;
		Node* node = buckets[buck]->next_node;
		while (node != Elist && node->cached == buck) {
			if (node->value.first == key) {
				return true;
			}
			node = node->next_node;
		}
		return false;
	}

	Node* __find(const Key& key) const noexcept{
		size_t buck = __hash(key);
		if (buckets[buck] == nullptr) return Elist;
		Node* node = buckets[buck]->next_node;
		while (node != Elist && node->cached == buck) {
			if (node->value.first == key) {
				return node;
			}
			node = node->next_node;
		}
		return Elist;
	}

	void __insert_in_empty_bucket(size_t buck, Node* insert_node) noexcept{
		insert_node->next_node = Elist->next_node;
		Elist->next_node = insert_node;
		buckets[buck] = Elist;
		if (insert_node->next_node != Elist) {
			buckets[insert_node->next_node->cached] = insert_node;
		}
	}

	void __insert_in_nonempty_bucket(size_t buck, Node* insert_node) noexcept {
		insert_node->next_node = buckets[buck]->next_node;
		buckets[buck]->next_node = insert_node;
	}

	void __rehash(size_t new_b_count) {
		Node** new_buckets = new Node*[new_b_count];
		for (size_t i = 0; i < new_b_count; ++i) {
			new_buckets[i] = nullptr;
		}

		Node* node = Elist;
		Node* next = node->next_node;
		Elist->next_node = Elist;
		while (next != Elist) {
			node = next;
			next = node->next_node;

			size_t buck = hasher(node->value.first) % new_b_count;
			node->cached = buck;
			if (new_buckets[buck] == nullptr) {
				
				node->next_node = Elist->next_node;
				Elist->next_node = node;
				new_buckets[buck] = Elist;
				if (node->next_node != Elist) {
					new_buckets[node->next_node->cached] = node;
				}
			}
			else {
				node->next_node = new_buckets[buck]->next_node;
				new_buckets[buck]->next_node = node;
			}
		}
		buckets_count = new_b_count;
		delete[] buckets;
		buckets = new_buckets;
	}



/**
 * @brief Функция вставки по ключу и значению
 * 
 * Вставляет введённые ключ и значение в таблицу
 * 
 * @param key - ключ, по которому будет проводиться вставка 
 * @param value - значение
 * @return iterator - итератор на вставленный элемент
 * 
 * \throw std::invalid_argument В случае если ключ уже был встречен в таблице
 * \throw std::bad_alloc В случае возникновения ошибки при выделении памяти
 * 
 * \warning При превышении _max_load_factor будет произведено увеличение количества корзин
 * \sa _max_load_factor
 * \sa buckets_count
 */
	iterator insert(const Key& key, const Value& value) {
		iterator it = insert(std::make_pair(key, value));
		return it;
	}	
	
	/**
	 * @brief функция вставки std::pair в unordered map
	 * 
	 * Вставляет пару ключ-значение в таблицу
	 * 
	 * @param value - вставляемая пара из ключа и значения
	 * @return iterator - итератор на вставленный элемент
	 * 
	 * \throw std::invalid_argument В случае если ключ уже был встречен в таблице
	 * \throw std::bad_alloc В случае возникновения ошибки при выделении памяти
	 */
	iterator insert(const std::pair<Key, Value>& value) {
		size_t buck = __hash(value.first);
		bool key_exist = __find_in_bucket(buck, value.first);
		if (key_exist) throw std::invalid_argument("Key repeat");

		Node* insert_node = new Node(value, buck);
		if (buckets[buck] == nullptr) __insert_in_empty_bucket(buck, insert_node);
		else __insert_in_nonempty_bucket(buck, insert_node);
		_size += 1;
		if (_size / buckets_count >= _max_load_factor) __rehash(buckets_count * 2);

		return iterator(insert_node);
	}

	/**
	 * @brief функция удаления по ключу элемента
	 * 
	 * Удаляет элемент с заданным ключом из таблицы
	 * 
	 * @param key Ключ, по которому будет произведено удаление
	 * \throw std::out_of_range В случае ненахождения данного ключа
	 */
	void erase(const Key& key) {
		size_t buck = __hash(key);
		Node* before = buckets[buck];
		Node* node = buckets[buck]->next_node;
		while (node != Elist && node->cached == buck) {
			if (node->value.first == key) {
				before->next_node = node->next_node;
				
				if (node->next_node == Elist && (before == Elist || before->cached != buck)) {
					buckets[buck] = nullptr;
				}
				else if (node->next_node != Elist && node->next_node->cached != buck) {
					buckets[node->next_node->cached] = before;
					if (before == Elist || before->cached != buck)
						buckets[buck] = nullptr;
				}
				delete node;
				_size -= 1;
				return;
			}
			before = node;
			node = node->next_node;
		}
		throw std::out_of_range("No such key");
	}

/**
 * @brief Функция очистки unordered map
 * 
 * Удаляет всю информацию, которая на данный момент хранилась в таблице
 * 
 */
	void clear() noexcept{
		if (Elist == nullptr) return;
		Node* node = Elist->next_node;
		Node* temp;
		while (node != Elist) {
			temp = node;
			node = node->next_node;
			buckets[temp->cached] = nullptr;
			delete temp;
		}
		Elist->next_node = Elist;
		_size = 0;
	}

/**
 * @brief Выделяет память под определённое количество "корзин" для хранения информации
 * 
 * @param new_buckets_count - новое количество "корзин" 
 * \throw std::invalid_argument В случае, если новое значение "корзин" меньше предыдущего
 * \sa size_t buckets_number()
 * \sa buckets_count
 * \throw std::bad_alloc В случае ошибок при выделении памяти
 */
	void reserve(size_t new_buckets_count) {
		if (new_buckets_count < buckets_count) throw std::invalid_argument("New size can't be less then the previous one");
		__rehash(new_buckets_count);
	}



public:
/**
 * @brief Получить значение коэффициента максимальной загруженности
 *
 * 
 * @return double максимальный коэффициент загруженности
 * 
 * \sa _max_load_factor
 */
	double max_load_factor() const noexcept {
		return _max_load_factor;
	}

	/**
	 * @brief Установить коэффициент максимальной загруженности
	 * 
	 * @param f Новое значение коэффициента 
	 * \throw std::invalid_argument В случае, если введёный коэффициент <= 0
	 * 
	 * \sa _max_load_factor
	 */
	void max_load_factor(double f) {
		if (f <= 0) throw std::invalid_argument("max_load_factor must be more then 0");
		_max_load_factor = f;
	}

	/**
	 * @brief Получить количество элементов в таблице
	 * 
	 * @return size_t Общее число элементов в таблице
	 */
	size_t size() const noexcept {
		return _size;
	}

	/**
	 * @brief Узнать, пустая ли таблица
	 * 
	 * @return true Таблица пустая
	 * @return false В табице есть элементы
	 */
	bool empty() const noexcept {
		return _size == 0;
	}

	/**
	 * @brief Узнать количество "корзин в таблице"
	 * 
	 * @return size_t Количество "корзин" в таблице
	 */
	size_t buckets_number() const noexcept{
		return buckets_count;
	}

/**
 * @brief Вывести содержимое в таблице в виде std::pair(key, value)
 * 
 * @param std::ostream& out Поток вывода (значение по умолчанию std::cout)
 * 
 * @return std::ostream& Указатель на поток вывода
 */
	std::ostream& show(std::ostream& out = std::cout) const noexcept {
		Node* node = Elist->next_node;
		while(node != Elist) {
			out << node->value.second << " ";
			node = node->next_node;
		}

		return out;
	}
	
	/**
	 * @brief Проверить две unordered map на равенство
	 * 
	 * @param other Unordered map, с которой должно быть произведено сравнение
	 * @return true Таблицы совпадают
	 * @return false Таблицы не совпадают
	 */
	bool operator==(const U_Map& other) const noexcept{
		if (_size != other._size) return false;

		Node* th = Elist->next_node;
		while (th != Elist) {
			bool find = false;
			size_t pos = th->cached;
			Node* ot = other.buckets[pos]->next_node;

			while (ot != other.Elist && ot->cached == pos) {
				if (th->value.first == ot->value.first) {
					find = true;
					break;
				}
				ot = ot->next_node;
			}
			if (!find) return false;
			th = th->next_node;
		}
		return true;
	}

	/**
	 * @brief Проверить две unordered map на неравенство
	 * 
	 * @param other Unordered map, с которой должно быть произведено сравнение
	 * @return true Таблицы не совпадают
	 * @return false Таблицы совпадают
	 */
	bool operator!= (const U_Map& other) const noexcept {
		return !(*this == other);
	}

public:
/**
 * @brief Узнать, есть ли элемент с данным ключом в таблице 
 * 
 * @param key Значение ключа, по которому будет произведён поиск
 * @return true Элемент с данным ключом есть в таблице
 * @return false Элемента с данным ключом нет в таблице
 */
	bool contains(const Key& key) const noexcept {
		size_t buck = __hash(key);
		return __find_in_bucket(buck, key);
	}

/**
 * @brief Получить итератор на пару ключ-значение в unordered map
 * 
 * @param key Ключ, по которому будет произведён поиск
 * @return iterator Итератор на найденную пару ключ-значение или end() в случае, если элемента с данным ключом нет
 * 
 */
	iterator find(const Key& key) {
		Node* node = __find(key);
		return iterator(node);
	}
	
/**
 * @brief Получить итератор на пару ключ-значение в const unordered map
 * 
 * @param key Ключ, по которому будет произведён поиск
 * @return const_iterator Константный итератор на найденную пару ключ-значение или cend() в случае, если элемента с данным ключом нет
 * 
 */
    const_iterator find(const Key& key) const {
		Node* node = __find(key);
		return const_iterator(node);
	}

/**
 * @brief Оператор обращения к элементу таблицы
 * 
 * Если элемент с данным ключом присутствует в таблице, то будет возвращена ссылка на значение, которое лежит по данному ключу.
 * Иначе будет произведена вставка элемента в таблицу с конструктором по умолчанию и последний будет вовзращён
 * @param key Ключ, по которому будет получен элемент
 * @return Value& Ссылка на значение, соответствующее заданному ключу или Value()
 * 
 * \throw std::bad_alloc При ошибке при выделении памяти (если был вставлен новый ключ)
 */
	Value& operator[](const Key& key) {
		iterator it = find(key);
		if (it == end()) {
			it = insert(key, Value());
			return it->second;
		}
		return it->second;
	}

/**
 * @brief Получить указатель на значение по ключу
 * 
 * @param key Ключ, по которому будет произведен поиск
 * @return Value& Ссылка на найденное значение
 * 
 * \throw std::out_of_range Если элемента с данным ключом нет
 */
	Value& at(const Key& key) {
		Node* node = __find(key);
		if (node == Elist) throw std::out_of_range("map out of range");
		return node->value.second;
	}
	
	/**
	 * @brief Получить указатель на значение по ключу в const unordered map
	 * 
	 * @param key Ключ, по которому будет произведен поиск
	 * @return const Value& Ссылка на найденное значение
	 * 
	 * \throw std::out_of_range Если элемента с данным ключом нет
	 */
	const Value& at(const Key& key) const{
		Node* node = __find(key);
		if (node == Elist) throw std::out_of_range("map out of range");
		return node->value.second;
	}

private:
/**
 * \private
 * @brief Класс итератор для unordered map.
 * 
 * Внутри таблицы расширяется в iterator и const_iterator
 * 
 * \sa iterator const_iterator
 * 
 * @tparam Item Шаблонный параметр
 */
	template <typename Item>
	class OwnIterator {
		/**
		 * \private
		 * @brief Указатель на текущий узел 
		 * 
		 */
		Node* cur;
	public:
	
		OwnIterator(Node* ptr = nullptr) : cur(ptr) {} ///< Конструктор от указателя на узел

		OwnIterator(const OwnIterator& other) : cur(other.cur) {} ///< Конструктор от существующего итератора

		/**
		 * @brief Оператор разыменовывания
		 * 
		 * @return Item& Ссылка на пару ключ-значение, которые хранятся по итератору
		 */
		Item& operator* () {
			return cur->value;
		}

		/**
		 * @brief Оператор ->
		 * 
		 * @return Item* Указатель на пару ключ-значение, которые хранятся по итератору
		 */
		Item* operator ->() {
			return &(cur->value);
		}

		/**
		 * @brief Оператор префиксного инкрементирования
		 * 
		 * @return OwnIterator& Ссылка на итератор
		 */
		OwnIterator& operator++() {
			cur = cur->next_node;
			return *this;
		}

		/**
		 * @brief Оператор постфиксного инкрементирования
		 * 
		 * Возвращает итератор на текущеее значение и инкрементирует итератор
		 * 
		 * @return OwnIterator 
		 */
		OwnIterator operator++(int) {
			Node* temp = cur;
			cur = cur->next_node;
			return OwnIterator<Item>(temp);
		}

		/**
		 * @brief Сравнение итераторов на равенство
		 * 
		 * @param other Итератор, с которым будет произведено сравнение
		 * @return true Итераторы равны (указывают на один и тот же узел)
		 * @return false Итераторы не равны
		 * 
		 * \sa Node
		 */
		bool operator==(const OwnIterator& other) const {
			return cur == other.cur;
		}

		/**
		 * @brief Сравнение итераторов на равенство
		 * 
		 * @param other Итератор, с которым будет произведено сравнение
		 * @return true Итераторы не равны (указывают на разные узлы)
		 * @return false Итераторы не равны
		 * 
		 * \sa Node
		 */
		bool operator!=(const OwnIterator& other) const {
			return cur != other.cur;
		}
	};
};